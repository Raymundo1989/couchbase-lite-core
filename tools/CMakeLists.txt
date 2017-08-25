cmake_minimum_required (VERSION 3.1)
cmake_policy(VERSION 3.1)
project (LiteCoreTools)

if(ANDROID)
    return()
endif()

set(TOP ${PROJECT_SOURCE_DIR}/../)

include_directories(${PROJECT_SOURCE_DIR}
                    ${TOP}C/include/
                    ${TOP}LiteCore/Support/
                    ${TOP}vendor/fleece/Fleece/)

### LITECORELOG

set(LITECORELOG_SRC  litecorelog.cc
               ${TOP}LiteCore/Support/StringUtil.cc
               ${TOP}LiteCore/Support/LogDecoder.cc
               ${TOP}LiteCore/Support/LogEncoder.cc)

if(MSVC)
    aux_source_directory(${TOP}MSVC MSVC_SOURCE)
    set(LITECORELOG_SRC ${LITECORELOG_SRC} ${MSVC_SOURCE})
endif()

add_executable(litecorelog
               ${LITECORELOG_SRC})

target_link_libraries(litecorelog  FleeceStatic)
if (APPLE)
    target_link_libraries(litecorelog "-framework CoreFoundation")
elseif(UNIX)
    target_link_libraries(litecorelog ${LIBCXX_LIB} ${LIBCXXABI_LIB})
endif()

### LITECP

aux_source_directory(litecp LITECP_SRC)

add_executable(litecp
               ${LITECP_SRC}
               ToolUtils.cc)

target_link_libraries(litecp ${LITECORE_LIBRARIES_PRIVATE})

if (APPLE)
    target_link_libraries(litecp 
      "-framework CoreFoundation"
      "-framework Security")
elseif(MSVC)
    target_link_libraries(litecp Dbghelp.lib)
else()
    target_link_libraries(litecp pthread z dl ${ICU4C_COMMON} ${ICU4C_I18N} ${LIBCXX_LIB} ${LIBCXXABI_LIB})
endif()