include(ExternalProject)

FetchContent_Declare(
    ext_implot
    PREFIX implot
    URL https://github.com/epezent/implot/archive/refs/tags/v0.14.tar.gz
    URL_HASH SHA256=1613af3e6554c0a74de20c6e60e9bce5ce35c2d4f9e1aa5ff963f7fe2d48af88
    DOWNLOAD_DIR "${OPEN3D_THIRD_PARTY_DOWNLOAD_DIR}/implot"
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

FetchContent_Populate(ext_implot)
FetchContent_GetProperties(ext_implot SOURCE_DIR IMPLOT_SOURCE_DIR)
