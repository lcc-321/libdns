cmake_minimum_required(VERSION 3.11)

project(lib_dns_lib VERSION 0.1)

if (NOT EXISTS "${LIBRESSL_ROOT_DIR}/lib/libssl.a")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" -S ${CMAKE_CURRENT_SOURCE_DIR}/ssl -B ${LIBRESSL_ROOT_DIR}-build -DCMAKE_INSTALL_PREFIX=${LIBRESSL_ROOT_DIR}
    RESULT_VARIABLE ret
  )
  if(${ret})
    message(FATAL_ERROR "ssl cmake configure error")
  endif()

  execute_process(
    COMMAND ${CMAKE_COMMAND} --build ${LIBRESSL_ROOT_DIR}-build
    RESULT_VARIABLE ret
  )
  if(${ret})
    message(FATAL_ERROR "ssl make error")
  endif()

  execute_process(
    COMMAND ${CMAKE_COMMAND} --install ${LIBRESSL_ROOT_DIR}-build
    RESULT_VARIABLE ret
  )
  if(${ret})
    message(FATAL_ERROR "ssl install error")
  endif()
endif()
