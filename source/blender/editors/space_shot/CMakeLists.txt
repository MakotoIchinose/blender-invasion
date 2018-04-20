set(INC
../include
../../blenfont
../../blenkernel
../../blenlib
../../blenloader
../../imbuf
../../gpu
../../makesdna
../../makesrna
../../windowmanager
../../../../intern/guardedalloc
../../../../intern/glew-mx
)
set(INC_SYS
${GLEW_INCLUDE_PATH}
)
set(SRC
space_shot.c
)

blender_add_lib(bf_editor_space_shot "${SRC}" "${INC}" "${INC_SYS}")
