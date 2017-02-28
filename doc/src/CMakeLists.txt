
# These macros loosely based on https://github.com/sabel83/mpllibs/blob/8e245aedcf658fe77bb29537aeba1d4e1a619a19/cmake/Pandoc.cmake
macro(pandoc_build_html TARGET_NAME OUTDIR)
  set(OUTFILES)
  foreach(INFILE ${ARGN})
    get_filename_component(FILENAME ${INFILE} NAME_WE)
    set(OUTFILE ${OUTDIR}/${FILENAME}.html)
    set(OUTFILES ${OUTFILES} ${OUTFILE})
    add_custom_command(
      OUTPUT ${OUTFILE}
      COMMAND
        ${PANDOC} --from markdown --to html -c style.css -o ${OUTFILE} ${IT}
      DEPENDS ${IT}
    )
  endforeach(IT)
  add_custom_target(${TARGET_NAME} DEPENDS ${OUTFILES})
endmacro(pandoc_build)

macro(pandoc_build_pdf TARGET_NAME OUTDIR)
  set(OUTFILES)
  foreach(INFILE ${ARGN})
    get_filename_component(FILENAME ${INFILE} NAME_WE)
    set(OUTFILE ${OUTDIR}/${FILENAME}.html)
    set(OUTFILES ${OUTFILES} ${OUTFILE})
    message( STATUS "outfile is ${OUTFILE}" )
    add_custom_command(
      OUTPUT ${OUTFILE}
      COMMAND
        ${PANDOC} --from markdown --to pdf -o ${OUTFILE} ${IT}
      DEPENDS ${IT}
    )
  endforeach(IT)
  add_custom_target(${TARGET_NAME} DEPENDS ${OUTFILES})
endmacro(pandoc_build)

pandoc_build_pdf( curation_pdf ${CMAKE_CURRENT_BINARY_DIR}/doc )
