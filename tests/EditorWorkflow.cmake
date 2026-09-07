# Separate executable invocations ensure loading cannot depend on author memory.
file(MAKE_DIRECTORY "${WORK}")
foreach(phase IN ITEMS author load)
    execute_process(COMMAND "${PROGRAM}" "${phase}" "${WORK}/scene.json"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 10)
    if(NOT result STREQUAL "0")
        message(FATAL_ERROR "${phase}: ${result}\n${output}\n${error}")
    endif()
    message(STATUS "${output}")
endforeach()
file(REMOVE "${WORK}/scene.json" "${WORK}/scene.json.copy" "${WORK}/scene.json.bad")
