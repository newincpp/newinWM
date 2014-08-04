SRC	=main.cpp \
	WindowManager.cpp
EXT     =cpp
NAME	=wm
WARNINGFLAGS= -Wall -Wextra -W
OPTIMFLAGS=-march=native -O2
CXXFLAGS= -std=c++0x $(WARNINGFLAGS) $(OPTIMFLAGS)
LDFLAGS	=-lX11
OBJS	= $(SRC:.$(EXT)=.o)
RM	= rm -f
CXX	= clang++
LINKER	= $(CXX)

all: $(NAME)

$(NAME): $(OBJS)
	$(LINKER) $(OBJS) $(LDFLAGS) -o $(NAME)

clean:
	$(RM) $(OBJS) *.swp *~ *#

fclean: clean
	$(RM) $(NAME)

re: make -B -j4

.PHONY: all clean fclean re

