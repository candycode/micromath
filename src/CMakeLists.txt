cmake_minimum_required(VERSION 2.6)

project( micromathplus )

set( DEF_INCLUDES adaptors.h compiler.h def_rte.h math_parser.h exception.h
     execution.h mmp_algorithm.h shared_ptr.h text_utility.h vm.h )  

set( DEF_SRCS test.cpp math_parser.cpp )

option( MMP_DEBUG_MEMORY "Enable memory tracer" OFF )

if( MMP_DEBUG_MEMORY )
  set( INCLUDES ${DEF_INCLUDES} dbgnew.h mem_tracer.h )
  set( SRCS ${DEF_SRCS} mem_tracer.cpp )
  set_source_files_properties( ${SRCS} COMPILE_FLAGS -DMMP_DEBUG_MEMORY )
else( MMP_DEBUG_MEMORY )
  set( INCLUDES ${DEF_INCLUDES} )
  set( SRCS ${DEF_SRCS} )
endif( MMP_DEBUG_MEMORY )      

add_executable( mmtest ${SRCS} ${INCLUDES} )