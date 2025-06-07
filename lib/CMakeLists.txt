# Build libressl directly
if (NOT EXISTS "${LIBRESSL_ROOT_DIR}/lib/libssl.a")
  execute_process(
    COMMAND ${CMAKE_COMMAND}
          -G "${CMAKE_GENERATOR}"
          -S ${CMAKE_CURRENT_SOURCE_DIR}/ssl
          -B ${LIBRESSL_ROOT_DIR}-build
          -DCMAKE_INSTALL_PREFIX=${LIBRESSL_ROOT_DIR}
          -DCMAKE_BUILD_TYPE=Release
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
