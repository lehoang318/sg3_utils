add_library(sgutils2 STATIC
    sg_lib.c
    sg_lib_data.c
    sg_cmds_basic.c
    sg_cmds_basic2.c
    sg_cmds_extra.c
    sg_cmds_mmc.c
    sg_pt_common.c

    # Platform-specific
    sg_io_linux.c
    sg_pt_linux.c
    sg_pt_linux_nvme.c
)
