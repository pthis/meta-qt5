require qt5.inc

SRC_URI += " \
  file://0001-qlibraryinfo-Allow-the-specification-of-where-to-loa.patch \
  file://0002-qmake-is-already-built-in-qt5-tools-native.patch \
  file://0003-Allow-building-a-separate-qmake-for-the-target.patch \
  file://qmake-build.conf.sh \
  file://qmake.conf.sh \
  file://qplatformdefs.h \
"

DEPENDS += "qt5-tools-native virtual/libgl freetype jpeg libpng zlib openssl glib-2.0 ${ICU}"

# to provide xcb/xcb_icccm.h, xcb/xcb_image.h, xcb/xcb_keysyms.h and dbus/dbus.h
# DEPENDS += "xcb-util-wm xcb-util-image xcb-util-keysyms dbus"
# reenable when we switch to "-qt-xcb" and build of 
# src/platformsupport/glxconvenience/ is resolved to be before build of
# src/plugins/platforms/xcb/

QT_MODULE_FLAGS = "-no-xcb"

INC_PR = "r0"

QT_BASE_NAME = "qt5"
QT_DIR_NAME = "qt5"

# Qt uses atomic instructions not supported in thumb mode
ARM_INSTRUCTION_SET = "arm"

inherit qmake5

do_configure_append() {
    ### Make sure that our mkspecs will eventually end up in STAGING_DATADIR
    ${OE_QMAKE_QMAKE} -set OE_CROSS_INSTALL_DATA ${datadir}/${QT_DIR_NAME}
    sed -i '/mkspecs\.path/s/QT_HOST_DATA/OE_CROSS_INSTALL_DATA/' qtbase.pro
    sed -i '/pritarget/s/QT_HOST_DATA/OE_CROSS_INSTALL_DATA/' mkspecs/features/qt_installs.prf

    ### Same applies to the module headers...
    ${OE_QMAKE_QMAKE} -set OE_CROSS_INSTALL_HEADERS ${includedir}/${QT_DIR_NAME}
    sed -i 's/QT_INSTALL_HEADERS/OE_CROSS_INSTALL_HEADERS/g' mkspecs/features/qt_installs.prf

    ### Create the mkspec for the target
    mkdir -p mkspecs/${TARGET_OS}-oe-g++
    cp -f ${WORKDIR}/qplatformdefs.h mkspecs/${TARGET_OS}-oe-g++
    bash ${WORKDIR}/qmake.conf.sh > mkspecs/${TARGET_OS}-oe-g++/qmake.conf

    ### Stage the base mkspecs so that 'HostData' will find them
    mkdir -p ${STAGING_DATADIR}/${QT_DIR_NAME}
    cp -r ${STAGING_DATADIR_NATIVE}/${QT_DIR_NAME}/mkspecs ${STAGING_DATADIR}/${QT_DIR_NAME}/
    ### Make sure that modules are installed correctly. During configure they will pick this one
    ### instead of the one from ${S}/mkspecs...
    sed -i '/pritarget/s/QT_HOST_DATA/OE_CROSS_INSTALL_DATA/' \
            ${STAGING_DATADIR}/${QT_DIR_NAME}/mkspecs/features/qt_installs.prf
    rm -rf ${STAGING_DATADIR}/${QT_DIR_NAME}/mkspecs/modules*

    ### Fix up prl and pkgconfig lib paths, by default they will point to /usr/lib
    ${OE_QMAKE_QMAKE} -set OE_CROSS_INSTALL_LIBS ${STAGING_LIBDIR}
    sed -i 's/QT_INSTALL_LIBS\/raw/OE_CROSS_INSTALL_LIBS/g' mkspecs/features/qt_module.prf
    sed -i 's/QT_INSTALL_LIBS\/raw/OE_CROSS_INSTALL_LIBS/g' ${STAGING_DATADIR}/${QT_DIR_NAME}/mkspecs/features/qt_module.prf

    ### Remove the old pri files
    rm ${STAGING_DATADIR}/${QT_DIR_NAME}/mkspecs/*.pri


    echo "[Paths]"                                              > $QT_CONF_PATH
    echo "Binaries=${bindir}"                                   >> $QT_CONF_PATH
    echo "Headers=${includedir}/${QT_DIR_NAME}"                 >> $QT_CONF_PATH
    echo "Documentation=${docdir}/${QT_DIR_NAME}"               >> $QT_CONF_PATH
    echo "Libraries=${libdir}"                                  >> $QT_CONF_PATH
    echo "Plugins=${libdir}/${QT_DIR_NAME}/plugins"             >> $QT_CONF_PATH
    echo "Data=${datadir}/${QT_DIR_NAME}"                       >> $QT_CONF_PATH
    echo "Translations=${datadir}/${QT_DIR_NAME}/translations"  >> $QT_CONF_PATH
    echo "Settings=${sysconfdir}/${QT_DIR_NAME}"                >> $QT_CONF_PATH
    echo "Examples=${bindir}/${QT_DIR_NAME}/examples"           >> $QT_CONF_PATH
    echo "HostBinaries=${STAGING_BINDIR_NATIVE}"                >> $QT_CONF_PATH
    echo "HostData=${STAGING_DATADIR}/${QT_DIR_NAME}"           >> $QT_CONF_PATH

    ./configure -v \
        -dont-process \
        -opensource -confirm-license \
        -prefix ${prefix} \
        -bindir ${bindir} \
        -libdir ${libdir} \
        -datadir ${datadir}/${QT_DIR_NAME} \
        -sysconfdir ${sysconfdir}/${QT_DIR_NAME} \
        -docdir ${docdir}/${QT_DIR_NAME} \
        -headerdir ${includedir}/${QT_DIR_NAME} \
        -plugindir ${libdir}/${QT_DIR_NAME}/plugins \
        -importdir ${libdir}/${QT_DIR_NAME}/imports \
        -translationdir ${datadir}/${QT_DIR_NAME}/translations \
        -examplesdir ${bindir}/${QT_DIR_NAME}/examples \
        -platform ${TARGET_OS}-oe-g++ \
        -xplatform ${TARGET_OS}-oe-g++ \
        ${QT_CONFIG_FLAGS}

    ### Since we are pointing our host data we need to get them there before compilation
    if [ ! -e ${STAGING_DATADIR}/${QT_DIR_NAME}/mkspecs/${TARGET_OS}-oe-g++ ]; then
        cp -rf mkspecs/${TARGET_OS}-oe-g++ ${STAGING_DATADIR}/${QT_DIR_NAME}/mkspecs/${TARGET_OS}-oe-g++
    fi
    ### qmodule.pri and qconfig.pri contain target specific stuff copy them over as well
    cp -f mkspecs/qmodule.pri ${STAGING_DATADIR}/${QT_DIR_NAME}/mkspecs/qmodule.pri
    cp -f mkspecs/qconfig.pri ${STAGING_DATADIR}/${QT_DIR_NAME}/mkspecs/qconfig.pri

    ### As we provided the '-dont-process' switch inorder to allow us to stage a few files
    ### generated by the configure we need to manually run qmake the generate _all_ the Makefiles (-r)
    ${OE_QMAKE_QMAKE} -r -d
}

do_compile_append() {
    # Build qmake for the target arch
    cd ${S}/qmake
    ${OE_QMAKE_QMAKE}
    oe_runmake CC="${CC}" CXX="${CXX}"
    cd ${S}
}

do_install_append() {
    ### Fix up the binaries to the right location
    ### TODO: FIX
    install -d ${D}${bindir}/
    mv ${D}/${STAGING_BINDIR_NATIVE}/* ${D}${bindir}/
    rm -rf ${D}/${STAGING_BINDIR_NATIVE}/

    # Install the right arch qmake
    rm ${D}/${bindir}/qmake
    install -m 0755 bin/qmake2 ${D}${bindir}/qmake
}

PACKAGES =. "${PN}-fonts "
FILES_${PN}-fonts = "${libdir}/fonts"