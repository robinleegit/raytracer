SRCS = \
	application/application.cpp \
	application/camera_roam.cpp \
	application/imageio.cpp \
	application/scene_loader.cpp \
	math/camera.cpp \
	math/color.cpp \
	math/math.cpp \
	math/matrix.cpp \
	math/quaternion.cpp \
	math/vector.cpp \
	raytracer/bvh.cpp \
	raytracer/geom_utils.cpp \
	raytracer/main.cpp \
	raytracer/raytracer.cpp \
	scene/geometry.cpp \
	scene/material.cpp \
	scene/mesh.cpp \
	scene/model.cpp \
	scene/scene.cpp \
	scene/sphere.cpp \
	scene/triangle.cpp \
	tinyxml/tinyxml.cpp \
	tinyxml/tinyxmlerror.cpp \
	tinyxml/tinyxmlparser.cpp \

# compiler flags
CXX = clang++
CXXFLAGS += -Wall -pedantic -I"./include" -I"./$(SRC_DIR)" -I"./$(TOP_OBJ_DIR)"
LDFLAGS = -lSDLmain -lSDL -lpng -lpthread
LDFLAGS += -lGL -lGLU -lboost_thread -lboost_system
ISPC = ispc
ISPCFLAGS = -O2 --target=avx-x2 --arch=x86-64

# src and executable
SRC_DIR = src
#SRCS = $(wildcard **/*.cpp)
TARGET = raytracer
# .o files
TOP_OBJ_DIR = bin
OBJ_DIR = $(TOP_OBJ_DIR)/$(SUB_OBJ_DIR)
OBJS = $(SRCS:.cpp=.o)
OBJS_PATH = $(addprefix $(OBJ_DIR)/,$(OBJS))
# .d files
DEPS = $(OBJS:.o=.d)
DEPS_PATH = $(addprefix $(OBJ_DIR)/,$(DEPS))
# ispc
ISPC_SRCS = raytracer/utils.ispc
ISPC_DEPS = $(addprefix $(SRC_DIR)/,$(ISPC_SRCS:.ispc=.h))
OBJS += $(ISPC_SRCS:.ispc=.o)

# default to release; other option is debug
ifeq ($(MODE),)
    MODE = release
endif

ifeq ($(MODE), release)
    SUB_OBJ_DIR = release
    CXXFLAGS += -O3
else ifeq ($(MODE), debug)
    SUB_OBJ_DIR = debug
    CXXFLAGS += -g -O0
endif

# targets
.PHONY: all clean fmt target

all: target

target: $(TARGET)

$(TARGET): $(OBJS_PATH)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.d: $(SRC_DIR)/%.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -MM -MP -MT $(@:.d=.o) -o $@ $<

# don't need to mkdir for object files since d files already exist
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.ispc
	$(ISPC) $(ISPCFLAGS) $< -o $@

$(SRC_DIR)/%.h: $(SRC_DIR)/%.ispc
	$(ISPC) $(ISPCFLAGS) $< -h $@

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS_PATH)
endif

clean:
	rm -rf $(TOP_OBJ_DIR) $(TARGET)

fmt:
	astyle --recursive --style=allman "*.cpp"
	astyle --recursive --style=allman "*.hpp"


