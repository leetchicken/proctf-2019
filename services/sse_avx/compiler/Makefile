OUTPUT=output
GENERATED=generated
CCARGS=-c -g -O0 -Wall -I antlr4-runtime/include -I $(GENERATED) -std=c++11	
LDARGS=-g
LIBS=antlr4-runtime/lib/libantlr4-runtime.a
CC=g++
GRAMMAR=VectorAssembler
ANTLR4=antlr4

ANTLRGEN=BaseListener Lexer Listener Parser 
OBJS=$(addsuffix .o,$(addprefix $(OUTPUT)/$(GRAMMAR),$(ANTLRGEN)))
GSOURCES=$(addsuffix .cpp,$(addprefix $(GENERATED)/$(GRAMMAR),$(ANTLRGEN)))

.precious: $(GSOURCES)

all: compiler

compiler: dirs antlr4 main.cpp $(OBJS)
	$(CC) $(CCARGS) frontend.cpp -o $(OUTPUT)/frontend.o 
	$(CC) $(CCARGS) main.cpp  -o $(OUTPUT)/main.o 
	$(CC) $(LDARGS) $(OUTPUT)/frontend.o $(OUTPUT)/main.o $(OBJS) $(LIBS) -o compiler

antlr4: $(GENERATED)/.generated;
 
$(GENERATED)/.generated: $(GRAMMAR).g4
	$(ANTLR4) -Dlanguage=Cpp -o $(GENERATED) $(GRAMMAR).g4
	@touch $(GENERATED)/.generated

$(OUTPUT)/%.o : $(GENERATED)/%.cpp
	$(CC) $(CCARGS) $< -o $@

$(GENERATED)/%.cpp: $(GENERATED)/.generated;

dirs:; mkdir -p $(OUTPUT) $(GENERATED) 
clean:; rm -rf $(OUTPUT) $(GENERATED)
