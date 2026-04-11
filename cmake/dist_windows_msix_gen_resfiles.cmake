get_filename_component(PARENT_DIR "${DIR}" DIRECTORY)

file(
    GLOB_RECURSE RESFILES
    LIST_DIRECTORIES FALSE
    RELATIVE "${PARENT_DIR}"
    "${DIR}/*")

string(REPLACE ";" "\n" RESFILES "${RESFILES}")
string(REPLACE "/" "\\" RESFILES "${RESFILES}")

file(WRITE "${RESFILES_PATH}" "${RESFILES}")
