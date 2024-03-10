CC := g++
HEADER_DIR := ./include
SRC_DIR := src
SRCS := $(wildcard $(SRC_DIR)/*)

# Define the C++ object files:
#
# This uses Suffix Replacement within a macro:
#   $(name:string1=string2)
#         For each word in 'name' replace 'string1' with 'string2'.
# Below we are replacing the suffix .cpp of all words in the macro SRCS
# with the .o suffix.
#
OBJS := $(SRCS:.cpp=.o)
INCLUDES := -I$(HEADER_DIR)
HEADER_FILES := $(wildcard $(HEADER_DIR)/*)

all: $(OBJS)
	$(CC) $(INCLUDES) -o snapshot $(OBJS)
	@echo  "Snapshot has been compiled"
# This is a suffix replacement rule for building .o's from .cpp's.
# It uses automatic variables $<: the name of the prerequisite of
# the rule(a .cpp file) and $@: the name of the target of the rule (a .o file).
# (See the gnu make manual section about automatic variables)
%.o: %.cpp $(HEADER_FILES)
	$(CC) $(INCLUDES) -c $< -o $@
clean:
	rm -rf $(SRC_DIR)/*.o
	rm snapshot
install:
	cp snapshot /bin/snapshot
	chown root /bin/snapshot
	chgrp root /bin/snapshot
	chmod 755 /bin/snapshot
	@echo "Snapshot utility has been installed"
uninstall:
	rm -rf /bin/snapshot
	@echo "Snapshot utility has been removed"
