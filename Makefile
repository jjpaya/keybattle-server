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

OPT_DBG += -Og -g -fsanitize=address
LD_DBG  += $(OPT_DBG)

CPPFLAGS += -std=c++20 -Wall -Wshadow -Wextra -pedantic-errors -Wno-unused-parameter -Wno-unused-variable -Wno-shadow -MMD -MP
LDFLAGS  += 

# Libraries
UWS_INCLUDES = -I ./lib/uWebSockets/src -I ./lib/uWebSockets/uSockets/src
UWS_LIBS = lib/uWebSockets/uSockets/uSockets.a
UWS_CFLAGS = -DUWS_WITH_PROXY

LIBRARIES = $(UWS_LIBS)

CPPFLAGS += -I ./src/ $(UWS_INCLUDES) $(UWS_CFLAGS)
LDFLAGS += $(UWS_LIBS)
LDLIBS += $(LIBRARIES) -lz

.PHONY: all dbg rel static clean

all: dbg

dbg: CPPFLAGS += $(OPT_DBG)
dbg: LDFLAGS += $(LD_DBG)
dbg: $(LIBRARIES) $(TARGET)

rel: CPPFLAGS += $(OPT_REL)
rel: LDFLAGS  += $(LD_REL)
rel: $(LIBRARIES) $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

.SECONDEXPANSION:
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $$(@D)
	$(CXX) $(CPPFLAGS) -c -o $@ $<

$(OUT_DIR) $(patsubst %/,%,$(sort $(dir $(OBJ_FILES)))):
	@mkdir -p $@


lib/uWebSockets/uSockets/uSockets.a:
	make -C lib/uWebSockets/uSockets

clean:
	- $(RM) -r $(OBJ_DIR) $(OUT_DIR)

-include $(DEP_FILES)
