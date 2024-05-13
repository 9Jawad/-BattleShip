CXXFLAGS += -std='c++20' -Wall -Wextra -Wsign-compare -fmax-errors=2 -pedantic -g
# Sur Mac, supprimer la ligne suivante
#CXXFLAGS += -Walloc-zero -Wctor-dtor-privacy -Wdeprecated-copy-dtor -Wduplicated-branches -Wduplicated-cond -Wextra-semi -Wfloat-equal -Wformat-signedness -Winit-self -Wlogical-op -Wnon-virtual-dtor -Wnull-dereference -Wold-style-cast -Woverloaded-virtual -Wsign-promo -Wstrict-null-sentinel -Wsuggest-attribute=const -Wsuggest-override -Wswitch-default -Wswitch -Wundef -Wuseless-cast -Wvolatile -Wzero-as-null-pointer-constant -fmax-errors=2 -Wformat=2 -fsanitize=undefined,address,leak
CXX = g++-10

.PHONY: default
default: battleshipServer battleshipClient

# ------ Compile project CLIENT

CLT_DIR = ./src/client
BUILD_CLT_DIR = ./build/client

HEADERS_CLT = $(wildcard ${CLT_DIR}**/*.hh)
SOURCES_CLT = $(wildcard ${CLT_DIR}**/*.cc)
OBJECTS_CLT = $(patsubst ${CLT_DIR}%.cc,${BUILD_CLT_DIR}%.o,${SOURCES_CLT})
DEPENDS_CLT = $(patsubst ${CLT_DIR}%.cc,${BUILD_CLT_DIR}%.d,$(SOURCES_CLT))

battleshipClient: ${OBJECTS_CLT}
	${CXX} ${CXXFLAGS} ${LDFLAGS} $^ -o $@ ${LOADLIBES} ${LDLIBS}

# Need to recompile on header change
-include $(DEPENDS_CLT)


${BUILD_CLT_DIR}%.o: ${CLT_DIR}%.cc Makefile
	@mkdir -p $(dir $@)
	${CXX} ${CXXFLAGS} ${LDFLAGS} -MMD -MP -c $< -o $@ ${LOADLIBES} ${LDLIBS}

# ------ Compile project SERVER

SRV_DIR = ./src/server
BUILD_SRV_DIR = ./build/server

HEADERS_SRV = $(wildcard ${SRV_DIR}**/*.hh)
SOURCES_SRV = $(wildcard ${SRV_DIR}**/*.cc)
OBJECTS_SRV = $(patsubst ${SRV_DIR}%.cc,${BUILD_SRV_DIR}%.o,${SOURCES_SRV})
DEPENDS_SRV = $(patsubst ${SRV_DIR}%.cc,${BUILD_SRV_DIR}%.d,$(SOURCES_SRV))

# Chemin vers le répertoire contenant sqlite3.h
SQLITE_INCLUDE_DIR = ./lib/sqlite3
SQLITE_LIB_PATH = ./lib/sqlite3/sqlite3.a
LIB_SRV= -lpthread -ldl

# Need to recompile on header change
-include $(DEPENDS_SRV)

battleshipServer: ${OBJECTS_SRV} $(SQLITE_LIB_PATH)
	${CXX} ${CXXFLAGS} ${LDFLAGS} $^ -o $@ ${LOADLIBES} ${LDLIBS} $(LIB_SRV)

$(SQLITE_LIB_PATH): ./lib/sqlite3/sqlite3.c ./lib/sqlite3/sqlite3.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Isqlite3 -c -o $(SQLITE_LIB_PATH) $<


${BUILD_SRV_DIR}%.o: ${SRV_DIR}%.cc Makefile
	@mkdir -p $(dir $@)
	${CXX} ${CXXFLAGS} ${LDFLAGS} -MMD -MP -c $< -o $@ ${LOADLIBES} ${LDLIBS} -I$(SQLITE_INCLUDE_DIR)


# make clean supprime les fichiers objets et dépendances
.PHONY: clean
clean:
	-rm ${BUILD_CLT_DIR}**/*.o ${BUILD_CLT_DIR}**/*.d
	-rm ${BUILD_SRV_DIR}**/*.o ${BUILD_SRV_DIR}**/*.d
	-rm $(SQLITE_LIB_PATH)

# make mrclean supprime les fichiers objets et les exécutables
.PHONY: mrclean
mrclean: clean
	-rm battleshipServer
	-rm battleshipClient