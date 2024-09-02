/*
 *  vfb2.c -- Virtual frame buffer device with pollable output.
 *
 *  Extended version of linux/drivers/video/vfb.c -- Virtual frame buffer device
 *
 *      Copyright (C) 2021 RSP Systems <software@rspsystems.com>
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 *
 */

/**
 *  This driver adds an extra character device /dev/fb_view which can be polled by
 *  a user application to get notified whenever panning is performed.
 *  Reading from the file always returns the content of struct fb_var_screeninfo,
 *  which contains the yoffset parameter used to control double buffering.
 *
 *  The emul_fb application is designed to show the content from this frame buffer
 *  in a native desktop window. This way it is possible to test gui frameworks
 *  utilizing a Linux frame buffer, still used in many embedded devices, from a
 *  standard desktop.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>

#include <linux/fb.h>
#include <linux/init.h>

#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>

/*#define PRINT(a, ...) pr_info(a, ##__VA_ARGS__)
 */
#define PRINT(a, ...)

    /*
     *  RAM we reserve for the frame buffer. This defines the maximum screen
     *  size
     *
     *  The default can be overridden if the driver is compiled as a module
     */

#define VIDEOMEMSIZE    (16 * 1024 * 1024)   /* 16 MB */

static void *videomemory;
static u_long videomemorysize = VIDEOMEMSIZE;
module_param(videomemorysize, ulong, 0);
MODULE_PARM_DESC(videomemorysize, " RAM available to frame buffer (in bytes). Defaults to 16MB");

static char *mode_option = NULL;
module_param(mode_option, charp, 0);
MODULE_PARM_DESC(mode_option, "Preferred video mode (e.g. 480x800-32@60)");

static const struct fb_videomode vfb_default = {
    .xres =     480,
    .yres =     800,
    .pixclock = 4883,
    .left_margin =  64,
    .right_margin = 64,
    .upper_margin = 32,
    .lower_margin = 32,
    .hsync_len =    64,
    .vsync_len =    2,
    .vmode =    FB_VMODE_NONINTERLACED,
};

static struct fb_fix_screeninfo vfb_fix = {
    .id =       "Virtual FB 2",
    .type =     FB_TYPE_PACKED_PIXELS,
    .visual =   FB_VISUAL_DIRECTCOLOR,
    .xpanstep = 1,
    .ypanstep = 1,
    .ywrapstep =    1,
    .accel =    FB_ACCEL_NONE,
};

static int vfb_check_var(struct fb_var_screeninfo *var,
             struct fb_info *info);
static int vfb_set_par(struct fb_info *info);
static int vfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
             u_int transp, struct fb_info *info);
static int vfb_pan_display(struct fb_var_screeninfo *var,
               struct fb_info *info);
static int vfb_mmap(struct fb_info *info,
            struct vm_area_struct *vma);

static const struct fb_ops vfb_ops = {
    .fb_read        = fb_sys_read,
    .fb_write       = fb_sys_write,
    .fb_check_var   = vfb_check_var,
    .fb_set_par = vfb_set_par,
    .fb_setcolreg   = vfb_setcolreg,
    .fb_pan_display = vfb_pan_display,
    .fb_fillrect    = sys_fillrect,
    .fb_copyarea    = sys_copyarea,
    .fb_imageblit   = sys_imageblit,
    .fb_mmap        = vfb_mmap
};

#define DEVICE_NAME "fb_view"

static int majorNumber;
static struct class*  viewClass  = NULL;
static struct device* viewDevice = NULL;

static DECLARE_WAIT_QUEUE_HEAD(pan_wait);
static int panned = 0;
static struct fb_var_screeninfo *fb_var_info;
static struct mutex view_mutex;

static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static unsigned int dev_poll(struct file *file, poll_table *wait);

static struct file_operations fops =
{
    .owner = THIS_MODULE,
    .open = dev_open,
    .read = dev_read,
    .release = dev_release,
    .poll = dev_poll
};

    /*
     *  Internal routines
     */

static u_long get_line_length(int xres_virtual, int bpp)
{
    u_long length;

    length = xres_virtual * bpp;
    length = (length + 31) & ~31;
    length >>= 3;
    return (length);
}

    /*
     *  Setting the video mode has been split into two parts.
     *  First part, xxxfb_check_var, must not write anything
     *  to hardware, it should only verify and adjust var.
     *  This means it doesn't alter par but it does use hardware
     *  data from it to check this var.
     */

static int vfb_check_var(struct fb_var_screeninfo *var,
             struct fb_info *info)
{
    u_long line_length;

    /*
     *  FB_VMODE_CONUPDATE and FB_VMODE_SMOOTH_XPAN are equal!
     *  as FB_VMODE_SMOOTH_XPAN is only used internally
     */

    if (var->vmode & FB_VMODE_CONUPDATE) {
        var->vmode |= FB_VMODE_YWRAP;
        var->xoffset = info->var.xoffset;
        var->yoffset = info->var.yoffset;
    }

    /*
     *  Some very basic checks
     */
    if (!var->xres)
        var->xres = 1;
    if (!var->yres)
        var->yres = 1;
    if (var->xres > var->xres_virtual)
        var->xres_virtual = var->xres;
    if (var->yres > var->yres_virtual)
        var->yres_virtual = var->yres;
    if (var->bits_per_pixel <= 1)
        var->bits_per_pixel = 1;
    else if (var->bits_per_pixel <= 8)
        var->bits_per_pixel = 8;
    else if (var->bits_per_pixel <= 16)
        var->bits_per_pixel = 16;
    else if (var->bits_per_pixel <= 24)
        var->bits_per_pixel = 24;
    else if (var->bits_per_pixel <= 32)
        var->bits_per_pixel = 32;
    else
        return -EINVAL;

    if (var->xres_virtual < var->xoffset + var->xres)
        var->xres_virtual = var->xoffset + var->xres;
    if (var->yres_virtual < var->yoffset + var->yres)
        var->yres_virtual = var->yoffset + var->yres;

    /*
     *  Memory limit
     */
    line_length =
        get_line_length(var->xres_virtual, var->bits_per_pixel);
    if (line_length * var->yres_virtual > videomemorysize)
        return -ENOMEM;

    /*
     * Now that we checked it we alter var. The reason being is that the video
     * mode passed in might not work but slight changes to it might make it
     * work. This way we let the user know what is acceptable.
     */
    switch (var->bits_per_pixel) {
    case 1:
    case 8:
        var->red.offset = 0;
        var->red.length = 8;
        var->green.offset = 0;
        var->green.length = 8;
        var->blue.offset = 0;
        var->blue.length = 8;
        var->transp.offset = 0;
        var->transp.length = 0;
        break;
    case 16:        /* RGBA 5551 */
        if (var->transp.length) {
            var->red.offset = 0;
            var->red.length = 5;
            var->green.offset = 5;
            var->green.length = 5;
            var->blue.offset = 10;
            var->blue.length = 5;
            var->transp.offset = 15;
            var->transp.length = 1;
        } else {    /* RGB 565 */
            var->red.offset = 0;
            var->red.length = 5;
            var->green.offset = 5;
            var->green.length = 6;
            var->blue.offset = 11;
            var->blue.length = 5;
            var->transp.offset = 0;
            var->transp.length = 0;
        }
        break;
    case 24:        /* RGB 888 */
        var->red.offset = 0;
        var->red.length = 8;
        var->green.offset = 8;
        var->green.length = 8;
        var->blue.offset = 16;
        var->blue.length = 8;
        var->transp.offset = 0;
        var->transp.length = 0;
        break;
    case 32:        /* RGBA 8888 */
        var->red.offset = 0;
        var->red.length = 8;
        var->green.offset = 8;
        var->green.length = 8;
        var->blue.offset = 16;
        var->blue.length = 8;
        var->transp.offset = 24;
        var->transp.length = 8;
        break;

    default:
        return -EINVAL;
    }
    var->red.msb_right = 0;
    var->green.msb_right = 0;
    var->blue.msb_right = 0;
    var->transp.msb_right = 0;

    return 0;
}

/* This routine actually sets the video mode. It's in here where we
 * the hardware state info->par and fix which can be affected by the
 * change in par. For this driver it doesn't do much.
 */
static int vfb_set_par(struct fb_info *info)
{
    switch (info->var.bits_per_pixel) {
    case 1:
        info->fix.visual = FB_VISUAL_MONO01;
        break;
    case 8:
        info->fix.visual = FB_VISUAL_PSEUDOCOLOR;
        break;
    case 16:
    case 24:
    case 32:
        info->fix.visual = FB_VISUAL_TRUECOLOR;
        break;
    default:
        return -EINVAL;
    }

    info->fix.line_length = get_line_length(info->var.xres_virtual,
                        info->var.bits_per_pixel);

    return 0;
}

    /*
     *  Set a single color register. The values supplied are already
     *  rounded down to the hardware's capabilities (according to the
     *  entries in the var structure). Return != 0 for invalid regno.
     */

static int vfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
             u_int transp, struct fb_info *info)
{
    if (regno >= 256)   /* no. of hw registers */
        return 1;
    /*
     * Program hardware... do anything you want with transp
     */

    /* grayscale works only partially under directcolor */
    if (info->var.grayscale) {
        /* grayscale = 0.30*R + 0.59*G + 0.11*B */
        red = green = blue =
            (red * 77 + green * 151 + blue * 28) >> 8;
    }

    /* Directcolor:
     *   var->{color}.offset contains start of bitfield
     *   var->{color}.length contains length of bitfield
     *   {hardwarespecific} contains width of RAMDAC
     *   cmap[X] is programmed to (X << red.offset) | (X << green.offset) | (X << blue.offset)
     *   RAMDAC[X] is programmed to (red, green, blue)
     *
     * Pseudocolor:
     *    var->{color}.offset is 0 unless the palette index takes less than
     *                        bits_per_pixel bits and is stored in the upper
     *                        bits of the pixel value
     *    var->{color}.length is set so that 1 << length is the number of available
     *                        palette entries
     *    cmap is not used
     *    RAMDAC[X] is programmed to (red, green, blue)
     *
     * Truecolor:
     *    does not use DAC. Usually 3 are present.
     *    var->{color}.offset contains start of bitfield
     *    var->{color}.length contains length of bitfield
     *    cmap is programmed to (red << red.offset) | (green << green.offset) |
     *                      (blue << blue.offset) | (transp << transp.offset)
     *    RAMDAC does not exist
     */
#define CNVT_TOHW(val,width) ((((val)<<(width))+0x7FFF-(val))>>16)
    switch (info->fix.visual) {
    case FB_VISUAL_TRUECOLOR:
    case FB_VISUAL_PSEUDOCOLOR:
        red = CNVT_TOHW(red, info->var.red.length);
        green = CNVT_TOHW(green, info->var.green.length);
        blue = CNVT_TOHW(blue, info->var.blue.length);
        transp = CNVT_TOHW(transp, info->var.transp.length);
        break;
    case FB_VISUAL_DIRECTCOLOR:
        red = CNVT_TOHW(red, 8);    /* expect 8 bit DAC */
        green = CNVT_TOHW(green, 8);
        blue = CNVT_TOHW(blue, 8);
        /* hey, there is bug in transp handling... */
        transp = CNVT_TOHW(transp, 8);
        break;
    default:
        break;
    }
#undef CNVT_TOHW
    /* Truecolor has hardware independent palette */
    if (info->fix.visual == FB_VISUAL_TRUECOLOR) {
        u32 v;

        if (regno >= 16)
            return 1;

        v = (red << info->var.red.offset) |
            (green << info->var.green.offset) |
            (blue << info->var.blue.offset) |
            (transp << info->var.transp.offset);
        switch (info->var.bits_per_pixel) {
        case 8:
            break;
        case 16:
            ((u32 *) (info->pseudo_palette))[regno] = v;
            break;
        case 24:
        case 32:
            ((u32 *) (info->pseudo_palette))[regno] = v;
            break;
        default:
            return -EINVAL;
        }
        return 0;
    }
    return 0;
}

    /*
     *  Pan or Wrap the Display
     *
     *  This call looks only at xoffset, yoffset and the FB_VMODE_YWRAP flag
     */

static int vfb_pan_display(struct fb_var_screeninfo *var,
               struct fb_info *info)
{
    if (var->vmode & FB_VMODE_YWRAP) {
        if (var->yoffset >= info->var.yres_virtual ||
            var->xoffset)
            return -EINVAL;
    } else {
        if (var->xoffset + info->var.xres > info->var.xres_virtual ||
            var->yoffset + info->var.yres > info->var.yres_virtual)
            return -EINVAL;
    }

    mutex_lock(&view_mutex);

    info->var.xoffset = var->xoffset;
    info->var.yoffset = var->yoffset;
    if (var->vmode & FB_VMODE_YWRAP)
        info->var.vmode |= FB_VMODE_YWRAP;
    else
        info->var.vmode &= ~FB_VMODE_YWRAP;

    PRINT("Display panned. yoffset: %d, %d", info->var.yoffset, (var->vmode & FB_VMODE_YWRAP));

    panned = 1;

    wake_up_interruptible(&pan_wait);

    mutex_unlock(&view_mutex);
    usleep_range(500, 1000);

    return 0;
}

    /*
     *  Most drivers don't need their own mmap function
     */

static int vfb_mmap(struct fb_info *info,
            struct vm_area_struct *vma)
{
    return remap_vmalloc_range(vma, (void *)info->fix.smem_start, vma->vm_pgoff);
}

/* **********************************************************************
 * Device handlers for /dev/fb_view
 * **********************************************************************
 */

static int dev_open(struct inode *inodep, struct file *filep)
{
    int err = 0;

    PRINT("dev_open enter\n");

    mutex_lock(&view_mutex);
    panned = 0;
    mutex_unlock(&view_mutex);

    return err;
}

static int dev_release(struct inode *inodep, struct file *filep)
{
    int err = 0;

    PRINT("dev_release enter\n");

    mutex_lock(&view_mutex);
    panned = -1;
    mutex_unlock(&view_mutex);

    return err;
}

static unsigned int dev_poll(struct file *filep, poll_table *wait)
{
    unsigned int ret = 0;

    PRINT("dev_poll enter\n");

    poll_wait(filep, &pan_wait, wait);

    mutex_lock(&view_mutex);

    if (panned == 1) {
        ret = POLLIN | POLLRDNORM;
    }

    mutex_unlock(&view_mutex);

    PRINT("dev_poll exit. return(%u)\n", ret);

    return ret;
}


static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
    int remaining;
    u32 result;

    PRINT("dev_read enter. len(%u) offset(%u)\n", (u32)len, (u32)*offset);

    if (len > sizeof(struct fb_var_screeninfo)) {
        return -ENOBUFS;
    }
/*
    if (panned == 0) {
        if (filep->f_flags & O_NONBLOCK) {
            return -EAGAIN;
        }
    }

    while (panned != 1) {
        if (panned == -1) {
            return -ENODATA;
        }
        msleep(10);
    }
*/
    mutex_lock(&view_mutex);

    panned = 0;

    result = min((int)len, (int)sizeof(struct fb_var_screeninfo));

    PRINT("dev_read: Copying %u bytes of data\n", result);
    PRINT("dev_read. yoffset: %d", fb_var_info->yoffset);

    remaining = copy_to_user(buffer, fb_var_info, result);

    /* copy_to_user returns number of bytes that could NOT be copied: 0 = success. */
    if(0 != remaining) {
        mutex_unlock(&view_mutex);
        pr_err("copy_to_user failed. Remaining=(%d)\n", remaining);
        return -ENOBUFS;
    }

    *offset += result;

    mutex_unlock(&view_mutex);

    PRINT("dev_read exit. Return(%u)\n", result );

    return result;
}


/*
 *  Initialisation
 */
static int vfb_probe(struct platform_device *dev)
{
    struct fb_info *info;
    unsigned int size = PAGE_ALIGN(videomemorysize);
    int retval = -ENOMEM;

    /*
     * For real video cards we use ioremap.
     */
    if (!(videomemory = vmalloc_32_user(size))) {
        fb_err(info, "Unable to allocate video memory.\n");
        return retval;
    }

    info = framebuffer_alloc(sizeof(u32) * 256, &dev->dev);
    if (!info) {
        fb_err(info, "Unable to allocate framebuffer.\n");
        goto err;
    }

    info->screen_base = (char __iomem *)videomemory;
    info->fbops = (struct fb_ops*)&vfb_ops;

    if (!fb_find_mode(&info->var, info, mode_option,
              NULL, 0, &vfb_default, 32)){
        fb_err(info, "Unable to find usable video mode.\n");
        retval = -EINVAL;
        goto err1;
    }

    vfb_fix.smem_start = (unsigned long) videomemory;
    vfb_fix.smem_len = videomemorysize;
    info->fix = vfb_fix;
    info->pseudo_palette = info->par;
    info->par = NULL;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,16,0)
    info->flags = FBINFO_FLAG_DEFAULT;
#else
    info->flags = FBINFO_VIRTFB;
#endif
    retval = fb_alloc_cmap(&info->cmap, 256, 0);
    if (retval < 0) {
        fb_err(info, "Unable to allocate cmap.\n");
        goto err1;
    }

    retval = register_framebuffer(info);
    if (retval < 0) {
        fb_err(info, "Unable to register framebuffer.\n");
        goto err2;
    }
    platform_set_drvdata(dev, info);

    vfb_set_par(info);

    fb_info(info, "Virtual frame buffer device, using %ldK of video memory\n",
        videomemorysize >> 10);

    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0) {
        dev_err(&dev->dev, DEVICE_NAME " failed to register a major number\n");
        return majorNumber;
    }

#if LINUX_VERSION_CODE <= KERNEL_VERSION(6,3,0)
    viewClass = class_create(THIS_MODULE, DEVICE_NAME);
#else
    viewClass = class_create(DEVICE_NAME);
#endif
    if (IS_ERR(viewClass)) {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        dev_err(&dev->dev, "Failed to register device class\n");
        return PTR_ERR(viewClass);
    }

    viewDevice = device_create(viewClass, &dev->dev, MKDEV(majorNumber, 0), dev, DEVICE_NAME);
    if (IS_ERR(viewDevice)) {
        class_destroy(viewClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        dev_err(&dev->dev, "Failed to create the device\n");
        return PTR_ERR(viewDevice);
    }

    fb_var_info = &info->var;

    mutex_init(&view_mutex);

    return 0;
err2:
    fb_dealloc_cmap(&info->cmap);
err1:
    framebuffer_release(info);
err:
    vfree(videomemory);
    return retval;
}

static int vfb_remove(struct platform_device *dev)
{
    struct fb_info *info = platform_get_drvdata(dev);

    if (info) {
        unregister_framebuffer(info);
        vfree(videomemory);
        fb_dealloc_cmap(&info->cmap);
        framebuffer_release(info);

        device_destroy(viewClass, MKDEV(majorNumber, 0));
        class_unregister(viewClass);
        class_destroy(viewClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
    }
    return 0;
}

static struct platform_driver vfb_driver = {
    .probe  = vfb_probe,
    .remove = vfb_remove,
    .driver = {
        .name   = "vfb2",
    },
};

static struct platform_device *vfb_device;

static int __init vfb_init(void)
{
    int ret = 0;

    ret = platform_driver_register(&vfb_driver);

    if (!ret) {
        vfb_device = platform_device_alloc("vfb2", 0);

        if (vfb_device)
            ret = platform_device_add(vfb_device);
        else
            ret = -ENOMEM;

        if (ret) {
            platform_device_put(vfb_device);
            platform_driver_unregister(&vfb_driver);
        }
    }

    return ret;
}

module_init(vfb_init);

#ifdef MODULE
static void __exit vfb_exit(void)
{
    platform_device_unregister(vfb_device);
    platform_driver_unregister(&vfb_driver);
}

module_exit(vfb_exit);

MODULE_LICENSE("GPL");
MODULE_VERSION("0.1.0");
#endif              /* MODULE */

