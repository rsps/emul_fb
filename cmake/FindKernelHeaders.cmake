# Find the kernel release
execute_process(
        COMMAND uname -r
        OUTPUT_VARIABLE KERNEL_RELEASE
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Find Kernel Config
find_path(KERNELCONFIG_DIR
        include/config/auto.conf
        PATHS
            /lib/modules/${KERNEL_RELEASE}/build/
            /lib/modules/${KERNEL_RELEASE}/source/
        )


# Find the headers
find_path(KERNELHEADERS_DIR
        include/linux/user.h
        PATHS
            /lib/modules/${KERNEL_RELEASE}/build/
            /lib/modules/${KERNEL_RELEASE}/source/
        )

message(STATUS "Kernel release: ${KERNEL_RELEASE}")
message(STATUS "Kernel headers: ${KERNELHEADERS_DIR}")
message(STATUS "Kernel conf: ${KERNELCONFIG_DIR}")


if (KERNELHEADERS_DIR)
    set(KERNELHEADERS_INCLUDE_DIRS
        ${KERNELHEADERS_DIR}/include
        ${KERNELHEADERS_DIR}/arch/x86/include
        ${KERNELHEADERS_DIR}/arch/x86/include/generated
        ${KERNELHEADERS_DIR}/arch/x86/include/uapi
        ${KERNELHEADERS_DIR}/arch/x86/include/generated/uapi
        ${KERNELHEADERS_DIR}/include/uapi
        ${KERNELHEADERS_DIR}/ubuntu/include
        CACHE PATH "Kernel headers include dirs"
        )
    set(KERNELHEADERS_FOUND 1 CACHE STRING "Set to 1 if kernel headers were found")
else (KERNELHEADERS_DIR)
    set(KERNELHEADERS_FOUND 0 CACHE STRING "Set to 1 if kernel headers were found")
endif (KERNELHEADERS_DIR)

mark_as_advanced(KERNELHEADERS_FOUND)
