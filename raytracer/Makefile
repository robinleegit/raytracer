# directories

# srouce file diretory
SRC_DIR = src
# top-level output directory (for .o and .d files)
TOP_OBJ_DIR = bin

# default to debug
ifeq ($(MODE),)
	MODE = debug
endif

SRCS = \
	application/application.cpp \
	application/imageio.cpp \
	application/camera_roam.cpp \
	application/scene_loader.cpp \
	math/math.cpp \
	math/color.cpp \
	math/vector.cpp \
	math/quaternion.cpp \
	math/matrix.cpp \
	math/camera.cpp \
	scene/material.cpp \
	scene/mesh.cpp \
	scene/scene.cpp \
	scene/sphere.cpp \
	scene/triangle.cpp \
	scene/model.cpp \
	tinyxml/tinyxml.cpp \
	tinyxml/tinyxmlerror.cpp \
	tinyxml/tinyxmlparser.cpp \
	raytracer/main.cpp \
	raytracer/bvh.cpp \
    raytracer/geom_utils.cpp \
	raytracer/raytracer.cpp

TARGET = raytracer


.PHONY: all clean

all: target

clean:
	rm -rf $(TOP_OBJ_DIR) $(TARGET)

UNAME := $(shell uname)

#########
# USAGE #
#########
# the makefile assumes the following variables are defined:
# TOP_OBJ_DIR: the top object level directory
# MODE: the mode, either "debug" or "release"
# SRCS: the source files
# TARGET: the target (executable) name
# MODE = release

ifeq ($(TARGET),)
ERRORMSG = "No target specified"
else ifeq ($(TOP_OBJ_DIR),)
ERRORMSG = "No object directory specified"
else ifeq ($(SRCS),)
ERRORMSG = "No sources specified"
endif

# the current directory
CURR_DIR = $(shell pwd)

# global compiler flags
CXX = clang++

CXXFLAGS += -Wall -pedantic -I"$(CURR_DIR)/include" -I"$(CURR_DIR)/$(SRC_DIR)"

LDFLAGS = -lSDLmain -lSDL -lpng -lpthread

ifeq ($(UNAME),Darwin)
LDFLAGS += -L/usr/local/Cellar/libpng12/1.2.50/lib -framework OpenGL -framework GLUT -lboost_thread-mt -lboost_system-mt
else
LDFLAGS += -lGL -lGLU -lboost_thread -lboost_system
endif

# object directories, mode flags
ifeq ($(UNAME),Darwin)
CPPFLAGS = -I/usr/local/Cellar/libpng12/1.2.50/include
endif

ifeq ($(MODE), release)
	SUB_OBJ_DIR = release
	CXXFLAGS += -O2
else ifeq ($(MODE), debug)
	SUB_OBJ_DIR = debug
	CXXFLAGS += -g -O0
else
ERRORMSG = "unknown build mode: $(MODE)"
endif

OBJ_DIR = $(TOP_OBJ_DIR)/$(SUB_OBJ_DIR)

# list of all object files
OBJS = $(SRCS:.cpp=.o)
# list of all dep files
DEPS = $(OBJS:.o=.d)
# full list of paths to all objs
OBJS_FULL = $(addprefix $(OBJ_DIR)/,$(OBJS))
# full list of paths to all deps
DEPS_FULL = $(addprefix $(OBJ_DIR)/,$(DEPS))

# sanity check for '.cpp' suffix
TMP_SRCS_NOT_CPP = $(filter-out %.cpp,$(SRCS))
ifneq (,$(TMP_SRCS_NOT_CPP))
ERRORMSG = "Feeling nervous about '$(TMP_SRCS_NOT_CPP)'; I only know how to build .cpp files"
endif

# targets

.PHONY: target

ifneq ($(ERRORMSG),)
target:
	$(error $(ERRORMSG))
else
target: $(OBJ_DIR)/$(TARGET).exe
	cp $< $(TARGET)

$(OBJ_DIR)/%.d: $(SRC_DIR)/%.cpp
	mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MM -MP -MT $(@:.d=.o) -o $@ $<

# don't need to mkdir for object files since d files already exist
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS_FULL)
endif

$(OBJ_DIR)/$(TARGET).exe: $(OBJS_FULL)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
endif

.PHONY: fmt

fmt:
	astyle --recursive --style=allman "*.cpp"
	astyle --recursive --style=allman "*.hpp"
