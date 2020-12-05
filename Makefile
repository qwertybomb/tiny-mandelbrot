NAME = prog
CC = clang-cl # if you don't have clang-cl use cl instead and change FLAGS
FLAGS = -m32 -W4 -c -nologo -Ofast -GS- -Gr -Oi -Gs9999999 -I .\khronos_headers
LINK_FLAGS = /HASHSIZE:10 main.obj kernel32.lib user32.lib shell32.lib gdi32.lib opengl32.lib \
			 /SUBSYSTEM:windows /NODEFAULTLIB /ENTRY:entry /OUT:"$(NAME).exe" /STACK:0x100000,0x100000


all: main.c
	$(CC) $(FLAGS) main.c && Crinkler $(LINK_FLAGS)

clean:
	del $(NAME).exe