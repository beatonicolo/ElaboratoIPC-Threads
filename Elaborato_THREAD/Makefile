# Sources
SRCS= sources/father.c sources/mylib.c sources/son.c
OBJS=$(SRCS:.c=.o)
EXECUTABLE=program.x

# Config
CC=gcc
CFLAGS= -c -Iheaders
LD=gcc
LDLIBS= -lpthread
# Target

all: program.x
	@echo "> Esegui con './program.x files/matA.txt files/matB.txt files/matC.txt ordineMatrice' o 'make run'"

program.x: $(OBJS)
	@echo Linking $@
	@$(LD) -o $@ $^ $(LDLIBS)

%.o: %.c
	@$(CC) $(CFLAGS) -o $@ $<  

#cleaning
clean:
	rm -f $(EXECUTABLE)
	rm -f $(OBJS)
#build and run
run: program.x
	@echo "> Esecuzione..."
	@sleep 2
	@./program.x files/matA.txt files/matB.txt files/matC.txt 8
#documentation
doc:
	@echo "> Creazione documentazione in corso... "
	@cd documentation;doxygen Doxyfile
	@cd documentation;ln -sf html/index.html ./documentazione.html
	@echo "> Per consultare la documentazione aprire il file 'documentazione.html' nella cartella documentation"
#delete the documentation
cleandoc:
	@echo -n "> Eliminazione della documentazione in corso... "
	@cd documentation;rm -rf html
	@cd documentation;rm -f documentazione.html
	@echo "Fatto"

.PHONY: all clean run doc cleandoc
