#pragma once

// "Diagnostic" vs "Message"? Something that includes errors+warnings?
// Have a stream each for errors/warnings?
enum MessageEnum : uint8_t {
    Message_LexError = 0,
    Message_StaticAssertFailed = 1,
    Message_IntLiteralOver64Bits = 2,
    Message_NoSignWrapViolated = 3, // nsw
};

struct Message {
    MessageEnum type;
    uint8_t miscU8;
    uint16_t column;
    int32_t line;
    // char *str;
};

class MessageStream {
    uint nMessages = 0; // ctor
    Message messages[256];

public:
    Message *PushRaw();
    // char *AllocString(uint nbytes); 
    const Message *begin() const { return messages; }
    const Message *end() const { return messages + nMessages; }
    uint size() const { return nMessages; }
    void clear() { nMessages = 0; }
};
