MAKE=make
CXX=g++
CXXFLAGS= -Iinclude -ISeAtS/include -g -pg -Wall -Werror -Wextra -O0 -DRLOG_COMPONENT="seats"
BINDIR=bin
OBJDIR=build
SRCDIR=src

LDFLAGS= -L./SeAtS/target/lib -lseats -lcrypto -lssl 

all: client server

lib: $(SRCDIR)/proxy_parse.c	
	@ mkdir -p $(OBJDIR)
	@ mkdir -p $(BINDIR)
	@ echo "Building seats library..."
	@ $(MAKE) -C SeAtS lib
	@ $(CXX) -o $(OBJDIR)/proxy_parse.o -c $(SRCDIR)/proxy_parse.c $(CXXFLAGS)

client: lib $(SRCDIR)/client_proxy.cpp 
	@ echo "Building client proxy..."
	@ $(CXX) -o $(OBJDIR)/client_proxy.o -c $(SRCDIR)/client_proxy.cpp $(CXXFLAGS)
	@ $(CXX) -o $(BINDIR)/client_proxy $(OBJDIR)/proxy_parse.o $(OBJDIR)/client_proxy.o $(LDFLAGS) 

server: lib $(SRCDIR)/server_proxy.cpp 
	@ echo "Building server proxy..."
	@ $(CXX) -o $(OBJDIR)/server_proxy.o -c $(SRCDIR)/server_proxy.cpp $(CXXFLAGS)
	@ $(CXX) -o $(BINDIR)/server_proxy $(OBJDIR)/proxy_parse.o $(OBJDIR)/server_proxy.o $(LDFLAGS) 
	
clean:
	@ $(MAKE) -C SeAtS clean
	@rm -rf $(BINDIR) $(OBJDIR)
