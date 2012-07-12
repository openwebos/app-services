LIBS := -lcurl -lglib-2.0 -lpalmsocket -llunaservice -licuuc $(LIBS) -ljemalloc_mt -lpthread -lsandbox -lPmLogLib -lboost_system -lz
LOCAL_LDFLAGS := $(LDFLAGS) $(LIBS)

SRCDIRS = activity async client data email exceptions glibcurl mimeparser network stream util
SOURCES = \
	activity/Activity.cpp \
	activity/ActivityBuilder.cpp \
	activity/ActivityParser.cpp \
	activity/ActivitySet.cpp \
	async/AsyncFlowControl.cpp \
	async/AsyncReader.cpp \
	async/AsyncWriter.cpp \
	async/AsyncInputStream.cpp \
	async/AsyncOutputStream.cpp \
	async/AsyncIOChannel.cpp \
	async/FileCacheResizerOutputStream.cpp \
	async/GIOChannelWrapper.cpp \
	client/BaseSyncSession.cpp \
	client/BusClient.cpp \
	client/Command.cpp \
	client/CommandManager.cpp \
	client/DbMergeProxy.cpp \
	client/FileCacheCleaner.cpp \
	client/FileCacheClient.cpp \
	client/SyncStateUpdater.cpp \
	client/PowerManager.cpp \
	client/PowerUser.cpp \
	data/DatabaseAdapter.cpp \
	data/Email.cpp \
	data/EmailPart.cpp \
	data/EmailAccount.cpp \
	data/EmailAccountAdapter.cpp \
	data/EmailAdapter.cpp \
	data/EmailAddress.cpp \
	data/EmailSchema.cpp \
	data/Folder.cpp \
	data/FolderAdapter.cpp \
	data/SyncState.cpp \
	data/SyncStateAdapter.cpp \
	email/AsyncEmailParser.cpp \
	email/AsyncEmailWriter.cpp \
	email/BufferedEmailParser.cpp \
	email/CharsetSupport.cpp \
	email/DateUtils.cpp \
	email/EmailWriter.cpp \
	email/HtmlEntities.cpp \
	email/MimeHeaderWriter.cpp \
	email/PreviewTextGenerator.cpp \
	email/Rfc2047Encoder.cpp \
	email/Rfc2047Decoder.cpp \
	exceptions/Backtrace.cpp \
	exceptions/CurlException.cpp \
	exceptions/ExceptionUtils.cpp \
	exceptions/GErrorException.cpp \
	exceptions/MailException.cpp \
	exceptions/MojErrException.cpp \
	glibcurl/glibcurl.cpp \
	mimeparser/EmailHeaderFieldParser.cpp \
	mimeparser/MimeEmailParser.cpp \
	mimeparser/Rfc822Tokenizer.cpp \
	network/CurlConnection.cpp \
	network/CurlSSLVerifier.cpp \
	network/HttpConnection.cpp \
	network/NetworkStatus.cpp \
	network/NetworkStatusMonitor.cpp \
	network/SocketConnection.cpp \
	stream/ByteBuffer.cpp \
	stream/BaseInputStream.cpp \
	stream/Base64DecoderOutputStream.cpp \
	stream/Base64OutputStream.cpp \
	stream/CounterOutputStream.cpp \
	stream/DebugOutputStream.cpp \
	stream/DeflaterOutputStream.cpp \
	stream/InflaterInputStream.cpp \
	stream/LineReader.cpp \
	stream/PreviewTextExtractorOutputStream.cpp \
	stream/QuotePrintableDecoderOutputStream.cpp \
	stream/QuotedPrintableEncoderOutputStream.cpp \
	stream/UTF8DecoderOutputStream.cpp \
	util/Hexdump.cpp \
	util/LogUtils.cpp \
	util/StringUtils.cpp \
	CommonErrors.cpp

OBJDIR  = $(BUILD_TYPE)-$(PLATFORM)
OBJDIRS = $(SRCDIRS:%=$(OBJDIR)/%)
OBJECTS = $(SOURCES:%.cpp=$(OBJDIR)/%.o)

SHARED_TARGET = libemail-common.so
SHARED_BIN = $(OBJDIR)/$(SHARED_TARGET)

all: $(OBJDIRS) $(SHARED_BIN)

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(OBJDIRS): $(OBJDIR)
	@mkdir -p $@

$(OBJDIR)/%.o: src/%.cpp
	mkdir -p $(@D)
	$(CXX) -MMD -MF $(patsubst %.o, %.d, $@) $(INCLUDES) $(LOCAL_CXXFLAGS) -c $< -o $@
	
$(SHARED_BIN): $(OBJECTS)
	@echo "Building $(SHARED_BIN)"
	$(CXX) $(LOCAL_CXXFLAGS) -shared -Wl,-soname,$(SHARED_TARGET) $(OBJECTS) $(LOCAL_LDFLAGS) -o $@

clean:
	rm -rf $(OBJDIR)

-include $(SOURCES:%.cpp=$(OBJDIR)/%.d)

# Test utils library
TEST_UTILS_SOURCES = \
	client/MockBusClient.cpp \
	client/MockRemoteService.cpp \
	client/MockFileCacheService.cpp
	
TEST_UTILS_TARGET = libemail-common-test.so
TEST_UTILS_BIN = $(OBJDIR)/$(TEST_UTILS_TARGET)

TEST_UTILS_OBJDIR = $(OBJDIR)/test-shared
TEST_UTILS_OBJDIRS = $(TEST_UTILS_DIRS:%=$(TEST_UTILS_OBJDIR)/%)
TEST_UTILS_OBJECTS := $(TEST_UTILS_SOURCES:%.cpp=$(TEST_UTILS_OBJDIR)/%.o)

$(TEST_UTILS_OBJDIR):
	@mkdir -p $(TEST_UTILS_OBJDIR)

$(TEST_UTILS_OBJDIRS): $(TEST_UTILS_OBJDIR)
	@mkdir -p $@

$(OBJDIR)/test-shared/%.o: test-shared/%.cpp
	mkdir -p $(@D)
	$(CXX) -MMD -MF $(patsubst %.o, %.d, $@) $(INCLUDES) $(TEST_INCLUDES) $(LOCAL_CXXFLAGS) -c $< -o $@

$(TEST_UTILS_BIN): $(TEST_UTILS_OBJECTS)
	@echo "Building $(TEST_UTILS_BIN)"
	$(CXX) $(LOCAL_CXXFLAGS) -shared -Wl,-soname,$(TEST_UTILS_TARGET) $(TEST_UTILS_OBJECTS) $(LOCAL_LDFLAGS) -L$(OBJDIR) -o $@

test-utils: $(TEST_UTILS_OBJDIRS) $(TEST_UTILS_BIN)

# Test targets
TEST_RUNNER := $(OBJDIR)/run_tests
	
TEST_DIRS = $(SRCDIRS)
	
TEST_SOURCES = \
	activity/ActivityTest.cpp \
	activity/ActivitySetTest.cpp \
	activity/MockActivityService.cpp \
	client/CommandManagerTest.cpp \
	data/FolderAdapterTest.cpp \
	email/AsyncEmailWriterTest.cpp \
	email/BufferedEmailParserTest.cpp \
	email/MimeHeaderWriterTest.cpp \
	email/MockAsyncIOChannel.cpp \
	email/PreviewTextGeneratorTest.cpp \
	email/Rfc2047DecoderTest.cpp \
	email/Rfc2047EncoderTest.cpp \
	mimeparser/EmailHeaderFieldParserTest.cpp \
	mimeparser/MimeEmailParserTest.cpp \
	mimeparser/Rfc822TokenizerTest.cpp \
	network/CurlConnectionTest.cpp \
	stream/UTF8DecoderOutputStreamTest.cpp \
	stream/QuotePrintableDecoderOutputStreamTest.cpp \
	stream/QuotedPrintableEncoderOutputStreamTest.cpp \
	stream/Base64OutputStreamTest.cpp \
	stream/ByteBufferOutputStreamTest.cpp \
	stream/CounterOutputStreamTest.cpp \
	stream/Base64DecoderOutputStreamTest.cpp \
	stream/DeflateInflateTest.cpp \
	stream/LineReaderTest.cpp \
	util/StringUtilsTest.cpp \
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
$(TEST_RUNNER): $(OBJECTS) $(TEST_OBJECTS) $(TEST_UTILS_OBJECTS)
	$(CXX) -o $(TEST_RUNNER) $(TEST_OBJECTS) $(TEST_UTILS_OBJECTS) $(OBJECTS) -lgtest -lmojocore -lmojodb $(LOCAL_LDFLAGS)

test: all test-utils $(TEST_OBJDIRS) $(TEST_RUNNER)

