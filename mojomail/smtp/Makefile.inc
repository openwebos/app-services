LIBS := -lpalmsocket -lemail-common -lboost_regex -lglib-2.0 $(LIBS) -ljemalloc_mt -lpthread -lmojodb
 
LOCAL_LDFLAGS := $(LDFLAGS) $(LIBS) -Wl,--unresolved-symbols=ignore-in-shared-libs

SRCDIRS = activity client commands data stream
	
SOURCES = \
	activity/SmtpActivityFactory.cpp \
	client/SmtpSession.cpp  \
	commands/SmtpCommand.cpp \
	commands/SmtpSessionCommand.cpp \
	commands/SmtpProtocolCommand.cpp \
	commands/AuthYahooCommand.cpp \
	commands/AuthLoginCommand.cpp \
	commands/AuthPlainCommand.cpp \
	commands/ConnectCommand.cpp \
	commands/ExtendedHelloCommand.cpp \
	commands/OldHelloCommand.cpp \
	commands/QuitCommand.cpp \
	commands/SimpleSendEmailCommand.cpp \
	commands/SmtpAccountDisableCommand.cpp \
	commands/SmtpAccountEnableCommand.cpp \
	commands/StartTlsCommand.cpp \
	commands/SaveEmailCommand.cpp \
	commands/AccountFinderCommand.cpp \
	data/MojoDatabase.cpp \
	data/SmtpAccount.cpp \
	data/SmtpAccountAdapter.cpp \
	stream/CRLFTerminatedOutputStream.cpp \
	SmtpClient.cpp \
	SmtpSimpleSender.cpp \
	SmtpValidator.cpp \
	SmtpBusDispatcher.cpp \
	SmtpConfig.cpp \
	commands/SmtpSendMailCommand.cpp \
	commands/SmtpSyncOutboxCommand.cpp \
	SmtpPowerManager.cpp

OBJECTS = $(SOURCES:%.cpp=$(OBJDIR)/%.o)
OBJDIR ?= $(BUILD_TYPE)-$(PLATFORM)
OBJDIRS = $(SRCDIRS:%=$(OBJDIR)/%)

SMTP_MAIN_SOURCE = SmtpServiceApp.cpp
SMTP_MAIN_OBJECT = $(SMTP_MAIN_SOURCE:%.cpp=$(OBJDIR)/%.o)

TARGET = $(OBJDIR)/mojomail-smtp

all: $(OBJDIRS) $(TARGET)

$(OBJDIRS):
	mkdir -p $@

$(OBJDIR)/%.o: src/%.cpp
	$(CXX) -MMD $(LOCAL_INCLUDES) $(LOCAL_CXXFLAGS) -c $< -o $@

$(TARGET): $(SMTP_MAIN_OBJECT) $(OBJECTS)
	$(CXX) -o $(TARGET) $(SMTP_MAIN_OBJECT) $(OBJECTS) $(LOCAL_INCLUDES) $(LOCAL_CXXFLAGS) $(LOCAL_LDFLAGS)

-include $(SOURCES:%.cpp=$(OBJDIR)/%.d)

clean:
	rm -Rf $(OBJDIR)
	
# Test targets
TEST_RUNNER := $(OBJDIR)/run_tests
	
TEST_DIRS = activity client commands data stream
TEST_SOURCES = \
	CRLFTerminatedOutputStreamTest.cpp \
	GTestMain.cpp
	
TEST_OBJDIR = $(OBJDIR)/test
TEST_OBJDIRS = $(TEST_DIRS:%=$(TEST_OBJDIR)/%)
TEST_OBJECTS := $(TEST_SOURCES:%.cpp=$(TEST_OBJDIR)/%.o)

$(TEST_OBJDIR):
	@mkdir -p $(TEST_OBJDIR)

$(TEST_OBJDIRS): $(TEST_OBJDIR)
	@mkdir -p $@ 

$(OBJDIR)/test/%.o: test/%.cpp
	$(CXX) $(LOCAL_INCLUDES) $(TEST_INCLUDES) $(LOCAL_CXXFLAGS) -c $< -o $@

# Link the test runner binary
$(TEST_RUNNER): $(TEST_OBJDIRS) $(OBJECTS) $(TEST_OBJECTS)
	$(CXX) -o $(TEST_RUNNER) $(TEST_OBJECTS) $(OBJECTS) -lgtest $(LOCAL_LDFLAGS)

test: all $(TEST_RUNNER)
