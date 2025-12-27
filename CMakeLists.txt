cmake_minimum_required(VERSION 3.16)
project(CleanLinuxpc VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/ui)

# البحث عن Qt
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Charts LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Charts LinguistTools)

# إعدادات الترجمة
set(TRANSLATIONS_DIR ${CMAKE_SOURCE_DIR}/resources/translations)
set(TS_FILES
    ${TRANSLATIONS_DIR}/cleanlinux_en.ts
    ${TRANSLATIONS_DIR}/cleanlinux_ar.ts
)

# توليد ملفات .qm من ملفات .ts
if(Qt${QT_VERSION_MAJOR}_VERSION VERSION_GREATER_EQUAL "5.15.0")
    qt_add_translation(QM_FILES ${TS_FILES})
else()
    qt5_add_translation(QM_FILES ${TS_FILES})
endif()

# نسخ ملفات .qm إلى مجلد resources/translations
foreach(qm_file ${QM_FILES})
    get_filename_component(qm_filename ${qm_file} NAME)
    add_custom_command(
        OUTPUT ${TRANSLATIONS_DIR}/${qm_filename}
        COMMAND ${CMAKE_COMMAND} -E copy ${qm_file} ${TRANSLATIONS_DIR}/${qm_filename}
        DEPENDS ${qm_file}
        COMMENT "Copying ${qm_filename} to resources/translations"
    )
    list(APPEND QM_FILES_IN_RESOURCES ${TRANSLATIONS_DIR}/${qm_filename})
endforeach()

# الملفات المصدرية
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/cleaner.cpp
    src/analyzer.cpp
    src/statsmodel.cpp
    src/settings.cpp
)

set(HEADERS
    src/mainwindow.h
    src/cleaner.h
    src/analyzer.h
    src/statsmodel.h
    src/settings.h
)

set(FORMS
    ui/mainwindow.ui
)

set(RESOURCES
    # resources/icons.qrc
    resources/translations.qrc
)

# إنشاء التطبيق
add_executable(CleanLinuxpc ${SOURCES} ${HEADERS} ${FORMS} ${RESOURCES} ${QM_FILES_IN_RESOURCES})

# الروابط
target_link_libraries(CleanLinuxpc
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Charts
)

# التثبيت
install(TARGETS CleanLinuxpc DESTINATION bin)
install(FILES packaging/com.linuxlaghouatalgeria.CleanLinuxpc.desktop DESTINATION share/applications)
install(FILES packaging/icons/com.linuxlaghouatalgeria.CleanLinuxpc.png DESTINATION share/icons/hicolor/256x256/apps)
install(FILES packaging/com.linuxlaghouatalgeria.CleanLinuxpc.metainfo.xml DESTINATION share/metainfo)
