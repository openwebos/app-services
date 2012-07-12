LIBS :=  -lemail-common -llunaservice -lmojodb $(shell pkg-config --libs glib-2.0)
LOCAL_LDFLAGS := $(LDFLAGS) $(LIBS) -Wl,--unresolved-symbols=ignore-in-shared-libs
SOURCE_DIRS := src data commands client
OBJDIR := $(BUILD_TYPE)-$(PLATFORM)
INCLUDES := $(LOCAL_INCLUDES) -I./inc 

POP_CLASSES := \
	activity/ActivityBuilderFactory.cpp \
	client/DownloadListener.cpp \
	client/PopSession.cpp \
	client/SyncSession.cpp \
	commands/AccountFinderCommand.cpp \
	commands/AutoDownloadCommand.cpp \
	commands/CheckTlsCommand.cpp \
	commands/ConnectCommand.cpp \
	commands/CreateUidMapCommand.cpp \
	commands/DeleCommand.cpp \
	commands/DeleteActivitiesCommand.cpp \
	commands/DeleteLocalEmailsCommand.cpp \
	commands/DeleteServerEmailsCommand.cpp \
	commands/DownloadEmailHeaderCommand.cpp \
	commands/DownloadEmailPartsCommand.cpp \
	commands/DownloadPartCommand.cpp \
	commands/FetchEmailCommand.cpp \
	commands/HandleRequestCommand.cpp \
	commands/InsertEmailsCommand.cpp \
	commands/ListCommand.cpp \
	commands/LoadUidCacheCommand.cpp \
	commands/LogoutCommand.cpp \
	commands/MoveEmailsCommand.cpp \
	commands/MovePopEmailsCommand.cpp \
	commands/NegotiateTlsCommand.cpp \
	commands/PasswordCommand.cpp \
	commands/PopAccountDeleteCommand.cpp \
	commands/PopAccountDisableCommand.cpp \
	commands/PopAccountEnableCommand.cpp \
	commands/PopAccountUpdateCommand.cpp \
	commands/PopClientCommand.cpp \
	commands/PopCommand.cpp \
	commands/PopCommandResult.cpp \
	commands/PopSessionCommand.cpp \
	commands/PopSessionPowerCommand.cpp \
	commands/PopMultiLineResponseCommand.cpp \
	commands/PopProtocolCommand.cpp \
	commands/PopProtocolCommandErrorResponse.cpp \
	commands/ReconcileEmailsCommand.cpp \
	commands/ScheduleRetryCommand.cpp \
	commands/SyncAccountCommand.cpp \
	commands/SyncEmailsCommand.cpp \
	commands/SyncFolderCommand.cpp \
	commands/TrimFolderEmailsCommand.cpp \
	commands/UidlCommand.cpp \
	commands/UpdateAccountStatusCommand.cpp \
	commands/UpdateUidCacheCommand.cpp \
	commands/UserCommand.cpp \
	data/DeletedEmailsCache.cpp \
	data/MojoDatabase.cpp \
	data/OldEmailsCache.cpp \
	data/PopAccount.cpp \
	data/PopAccountAdapter.cpp \
	data/PopEmail.cpp \
	data/PopEmailAdapter.cpp \
	data/PopFolder.cpp \
	data/PopFolderAdapter.cpp \
	data/UidCacheAdapter.cpp \
	data/UidCache.cpp \
	data/UidMap.cpp \
	request/Request.cpp \
	request/RequestManager.cpp \
	ParseEmlHandler.cpp \
	PopValidator.cpp \
	PopBusDispatcher.cpp \
	PopClient.cpp \
	PopErrors.cpp
	
POP_MAIN_SOURCE = PopMain.cpp
	
POP_CLASSES_OBJECTS := $(POP_CLASSES:%.cpp=$(OBJDIR)/%.o)
POP_INCLUDES := inc/*.h

POP_TARGET := $(OBJDIR)/mojomail-pop
POP_SOURCES := $(POP_CLASSES)
POP_OBJECTS := $(POP_SOURCES:%.cpp=$(OBJDIR)/%.o)
POP_MAIN_OBJECT = $(POP_MAIN_SOURCE:%.cpp=$(OBJDIR)/%.o)
OBJDIRS = $(SOURCE_DIRS:%=$(OBJDIR)/%)
OBJECTS = $(POP_SOURCES:%.cpp=$(OBJDIR)/%.o)

all: setup $(POP_TARGET)

$(POP_TARGET): $(POP_MAIN_OBJECT) $(POP_OBJECTS)
	$(CXX) -o $(POP_TARGET) $(POP_MAIN_OBJECT) $(POP_OBJECTS) $(LOCAL_LDFLAGS) 
	
$(OBJDIR)/%.o: src/%.cpp
	mkdir -p $(@D)
	$(CXX) -MMD -MF $(patsubst %.o, %.d, $@) -MMD $(INCLUDES) $(LOCAL_CFLAGS) $(LOCAL_CPPFLAGS) -c $< -o $@

setup: $(OBJDIRS)

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(OBJDIRS): $(OBJDIR)
	@mkdir -p $@

# Test targets
TEST_RUNNER := $(OBJDIR)/run_tests
	
TEST_DIRS = data
TEST_SOURCES = \
	data/TestOldEmailsCache.cpp \
	GTestMain.cpp
	
TEST_OBJDIR = $(OBJDIR)/test
TEST_OBJDIRS = $(TEST_DIRS:%=$(TEST_OBJDIR)/%)
TEST_OBJECTS := $(TEST_SOURCES:%.cpp=$(TEST_OBJDIR)/%.o)

$(TEST_OBJDIR):
	@mkdir -p $(TEST_OBJDIR)

$(TEST_OBJDIRS): $(TEST_OBJDIR)
	@mkdir -p $@ 

$(OBJDIR)/test/%.o: test/%.cpp
	mkdir -p $(@D)
	$(CXX) -MMD -MF $(patsubst %.o, %.d, $@) $(INCLUDES) $(TEST_INCLUDES) $(LOCAL_CXXFLAGS) -c $< -o $@

# Link the test runner binary
$(TEST_RUNNER): $(OBJECTS) $(TEST_OBJECTS)
	$(CXX) -o $(TEST_RUNNER) $(TEST_OBJECTS) $(OBJECTS) -lmojocore -lmojodb -lgtest $(LOCAL_LDFLAGS)

test: all $(TEST_OBJDIRS) $(TEST_RUNNER)

vpath %.cpp $(SOURCE_DIRS)

-include $(POP_SOURCES:%.cpp=$(OBJDIR)/%.d)
	
clean:
	rm -rf $(OBJDIR)
	rm -rf doc/html
	rm -rf test_runner test_runner.*
	
.PHONY: clean all setup docs
