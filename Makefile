# https://stackoverflow.com/a/18258352
rwildcard=$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

SRC_DIR = src

OBJ_DIR = build

SRC_FILES = $(call rwildcard, $(SRC_DIR)/, *.cpp)
OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEP_FILES = $(OBJ_FILES:.o=.d)

TARGET = keybattled

OPT_REL += -O2
LD_REL  += 

OPT_DBG += -Og -g
LD_DBG  += $(OPT_DBG)

CPPFLAGS += -std=c++17 -Wall -Wshadow -Weffc++ -Wextra -pedantic-errors -Wno-unused-parameter -MMD -MP
LDFLAGS  += 

CPPFLAGS += -I ./src/

.PHONY: all dbg rel static clean

all: dbg

dbg: CPPFLAGS += $(OPT_DBG)
dbg: LDFLAGS += $(LD_DBG)
dbg: $(TARGET)

rel: CPPFLAGS += $(OPT_REL)
rel: LDFLAGS  += $(LD_REL)
rel: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

.SECONDEXPANSION:
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $$(@D)
	$(CXX) $(CPPFLAGS) -c -o $@ $<

$(OUT_DIR) $(patsubst %/,%,$(sort $(dir $(OBJ_FILES)))):
	@mkdir -p $@

clean:
	- $(RM) -r $(OBJ_DIR) $(OUT_DIR)

-include $(DEP_FILES)
