#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <imgui.h>

struct Console
{
    enum class Result : unsigned char
    {
        None = 0,   /// Untyped, universal output
        Info,       /// Output reported an info (log)
        Warning,    /// Output reported a warning (strong note)
        Error,      /// Output reported an error (strong warning)
        Critical,   /// Output or backend threw an exception or reported a blocking error, cannot proceed, exit
        Success,    /// Output or backend reported that operation has succeded and can be terminated
        AutoFormat  /// No explicit type info is provided, should be determined by the formatter
    };

    struct Message
    {
        Result result;
        std::string msg;
    };

    void Print(const std::string& line, Result type = Result::None);
    
    /// <summary>
    /// Calls Print for each line. If type==AutoFormat, then each line will be formatted automatically, based on its contents
    /// </summary>
    void PrintMultiline(const std::string& line, Result type = Result::AutoFormat);

    /// <summary>
    /// Clears the entire console, deleting all messages
    /// </summary>
    void Clear();

    /// <summary>
    /// Main rendering function, also responsible for polling events in immediate GUI mode
    /// </summary>
    /// <param name="title">String representing the title of the docking window</param>
    /// <param name="open">Control bool to read/write whether the docking window is open</param>
    void Draw(const char* title, bool* open = nullptr);

    /// <summary>
    /// Analyzes the given line (or any text in general), finds first keyword and returns the type of that line based on that. If input contains multiple keywords (e.g. "Success: no errors"), first one will determine the type (in this case will be Console::Result::Success).
    /// </summary>
    Console::Result GetConsoleResult(const std::string& line);

    bool scrollToBottom = true;

private:
    struct Entry
    {
        std::string text;
        Result type;
    };

    std::vector<Entry> lines;
    std::mutex mutex;
    bool pendingScroll = true;

    /// <summary>
    /// Should INFO (DISPLAY) messages be outputted or collapsed into a counter?
    /// </summary>
    bool showDisplay = false;
    
    /// <summary>
    /// How many INFO (DISPLAY) messages were collapsed in a row. Only if showDisplay==true
    /// </summary>
    int displaysSkipped = 0;

    /// <summary>
    /// First click of the selection. FROM where to select. Not necessarily the first message in the selection.
    /// </summary>
    int selectionAnchor = -1;

    /// <summary>
    /// Second click of the selection. TO where to select. Not necessarily the last message in the selection.
    /// </summary>
    int selectionTail   = -1;

    /// <summary>
    /// Puts text of all selected lines combined into clipboard using imgui.
    /// </summary>
    void CopySelection();

    /// <summary>
    /// Special case of Print, which can collapse multiple messages of type Info into one, if showDisplay==false
    /// </summary>
    void PrintInfo(const std::string& line);
};