
LIBS := -lmojocore -lmojoluna -lemail-common -lglib-2.0 -lmojodb $(LIBS)
LOCAL_LDFLAGS := $(LDFLAGS) -Wl,--unresolved-symbols=ignore-in-shared-libs $(LIBS)

SRCDIRS = activity client commands connections data exceptions parser protocol sync
SOURCES = \
	ImapBusDispatcher.cpp \
	ImapClient.cpp \
	ImapConfig.cpp \
	ImapValidator.cpp \
	activity/ImapActivityFactory.cpp \
	client/Capabilities.cpp \
	client/DownloadListener.cpp \
	client/FolderSession.cpp \
	client/ImapRequestManager.cpp \
	client/ImapSession.cpp \
	client/SearchRequest.cpp \
	client/SyncParams.cpp \
	client/SyncSession.cpp \
	commands/AppendCommand.cpp \
	commands/AuthYahooCommand.cpp \
	commands/AutoDownloadCommand.cpp \
	commands/BaseIdleCommand.cpp \
	commands/CapabilityCommand.cpp \
	commands/CheckOutboxCommand.cpp \
	commands/CheckDraftsCommand.cpp \
	commands/CompressCommand.cpp \
	commands/ConnectCommand.cpp \
	commands/CreateDefaultFoldersCommand.cpp \
	commands/DisableAccountCommand.cpp \
	commands/DeleteAccountCommand.cpp \
	commands/DeleteActivitiesCommand.cpp \
	commands/DeleteFoldersCommand.cpp \
	commands/EnableAccountCommand.cpp \
	commands/FetchNewHeadersCommand.cpp \
	commands/FetchPartCommand.cpp \
	commands/HandleUpdateCommand.cpp \
	commands/IdleCommand.cpp \
	commands/IdleYahooCommand.cpp \
	commands/ImapAccountFinder.cpp \
	commands/ImapBusDispatcherCommand.cpp \
	commands/ImapClientCommand.cpp \
	commands/ImapCommand.cpp \
	commands/ImapCommandResult.cpp \
	commands/ImapSessionCommand.cpp \
	commands/ImapSyncSessionCommand.cpp \
	commands/LoginCommand.cpp \
	commands/LogoutCommand.cpp \
	commands/MoveEmailsCommand.cpp \
	commands/NoopIdleCommand.cpp \
	commands/PurgeEmailsCommand.cpp \
	commands/SearchFolderCommand.cpp \
	commands/SelectFolderCommand.cpp \
	commands/ScheduleRetryCommand.cpp \
	commands/SyncAccountCommand.cpp \
	commands/SyncEmailsCommand.cpp \
	commands/SyncFolderCommand.cpp \
	commands/SyncFolderListCommand.cpp \
	commands/SyncLocalChangesCommand.cpp \
	commands/TLSCommand.cpp \
	commands/UpdateAccountCommand.cpp \
	commands/UpdateAccountErrorCommand.cpp \
	commands/UpdateFolderActivitiesCommand.cpp \
	commands/UpSyncSentEmailsCommand.cpp \
	commands/UpSyncDraftsCommand.cpp \
	data/ImapAccount.cpp \
	data/ImapAccountAdapter.cpp \
	data/ImapEmail.cpp \
	data/ImapEmailAdapter.cpp \
	data/ImapFolder.cpp \
	data/ImapFolderAdapter.cpp \
	data/ImapLoginSettings.cpp \
	data/MojoDatabase.cpp \
	exceptions/Rfc3501ParseException.cpp \
	parser/ImapParser.cpp \
	parser/Parser.cpp \
	parser/Rfc3501Tokenizer.cpp \
	parser/SemanticActions.cpp \
	parser/Util.cpp \
	protocol/AppendResponseParser.cpp \
	protocol/BufferedResponseParser.cpp \
	protocol/CapabilityResponseParser.cpp \
	protocol/ExamineResponseParser.cpp \
	protocol/FetchResponseParser.cpp \
	protocol/ImapResponseParser.cpp \
	protocol/ListResponseParser.cpp \
	protocol/NamespaceResponseParser.cpp \
	protocol/StoreResponseParser.cpp \
	protocol/UidSearchResponseParser.cpp \
	protocol/UntaggedUpdateParser.cpp \
	sync/FolderListDiff.cpp \
	sync/SpecialFolderPicker.cpp \
	sync/SyncEngine.cpp \
	sync/UIDMap.cpp
	
IMAP_MAIN_SOURCE = ImapServiceApp.cpp

OBJDIR  = $(BUILD_TYPE)-$(PLATFORM)
OBJDIRS = $(SRCDIRS:%=$(OBJDIR)/%)
IMAP_OBJECTS = $(SOURCES:%.cpp=$(OBJDIR)/%.o)
IMAP_MAIN_OBJECT = $(IMAP_MAIN_SOURCE:%.cpp=$(OBJDIR)/%.o)
OBJECTS = $(IMAP_OBJECTS) $(IMAP_MAIN_OBJECT)

TARGET = $(OBJDIR)/mojomail-imap

all: $(OBJDIRS) $(TARGET)

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(OBJDIRS): $(OBJDIR)
	@mkdir -p $@

$(OBJDIR)/%.o: src/%.cpp
	mkdir -p $(@D)
	$(CXX) -MMD -MF $(patsubst %.o, %.d, $@) $(INCLUDES) $(LOCAL_CXXFLAGS) -c $< -o $@
	
$(TARGET): $(OBJECTS)
	@echo "Building $(TARGET)"
	$(CXX) $(LOCAL_CXXFLAGS) $(OBJECTS) $(LOCAL_LDFLAGS) -o $@

clean:
	rm -rf $(OBJDIR)

-include $(SOURCES:%.cpp=$(OBJDIR)/%.d)
-include $(IMAP_MAIN_SOURCE:%.cpp=$(OBJDIR)/%.d)

# Test targets
TEST_RUNNER := $(OBJDIR)/run_tests

TEST_DIRS = client commands data protocol sync
TEST_SOURCES = \
	client/ImapRequestManagerTest.cpp \
	client/MockImapClient.cpp \
	client/MockImapSession.cpp \
	client/MockSyncSession.cpp \
	commands/SyncLocalChangesCommandTest.cpp \
	protocol/ExamineResponseParserTest.cpp \
	protocol/FetchResponseParserTest.cpp \
	protocol/ListResponseParserTest.cpp \
	protocol/MockDoneSlot.cpp \
	protocol/UidSearchResponseParserTest.cpp \
	sync/FolderListDiffTest.cpp \
	sync/SpecialFolderPickerTest.cpp \
	sync/SyncEngineTest.cpp \
	sync/UIDMapTest.cpp \
	MockTestSetup.cpp \
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
	$(CXX) -MMD -MF $(patsubst %.o,%.d,$@) $(INCLUDES) $(TEST_INCLUDES) $(LOCAL_CXXFLAGS) -c $< -o $@
	
# Link the test runner binary
$(TEST_RUNNER): $(OBJECTS) $(TEST_OBJECTS)
	$(CXX) -o $(TEST_RUNNER) $(TEST_OBJECTS) $(TEST_RUNNER_OBJECT) $(IMAP_OBJECTS) $(LOCAL_LDFLAGS) -lgtest

test: all $(TEST_OBJDIRS) $(TEST_RUNNER)
	@# Do nothing

-include $(TEST_SOURCES:%.cpp=$(TEST_OBJDIR)/%.d)
