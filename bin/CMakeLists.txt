# executable has same name as lib, see https://gist.github.com/jlgerber/eafc4ee2b9954e27dd2bb009496b1b03
add_executable(base64-bin base64.c)
target_link_libraries(base64-bin PRIVATE base64)
install(TARGETS base64-bin DESTINATION bin)
set_target_properties(base64-bin PROPERTIES OUTPUT_NAME base64)
