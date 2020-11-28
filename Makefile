NAME = prog
CC = clang-cl # if you don't have clang-cl use cl instead and remove -m32
FLAGS = -m32 -W4 -c -nologo -O2 -GS- -Gr -Oi -Gs9999999 -I .\khronos_headers
LINK_FLAGS =  /HASHSIZE:10 main.obj kernel32.lib user32.lib shell32.lib gdi32.lib opengl32.lib \
			 /SUBSYSTEM:CONSOLE /NODEFAULTLIB /ENTRY:main /OUT:"$(NAME).exe" /STACK:0x100000,0x100000

all: main.c
	$(CC) $(FLAGS) main.c && Crinkler $(LINK_FLAGS)

clean:
	del $(NAME).exe