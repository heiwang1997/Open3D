include(ExternalProject)

ExternalProject_Add(
    ext_implot
    PREFIX implot
    URL https://github.com/epezent/implot/archive/refs/tags/v0.12.tar.gz
    URL_HASH SHA256=f8bc3b9b58dbabe3a0c0a2ebb8307d8e850012685332a85be36fcc4d4450be56
    DOWNLOAD_DIR "${OPEN3D_THIRD_PARTY_DOWNLOAD_DIR}/implot"
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(ext_implot SOURCE_DIR)
set(IMPLOT_SOURCE_DIR ${SOURCE_DIR})
