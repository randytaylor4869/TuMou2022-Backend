#ifndef _BOT_H_
#define _BOT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define BOT_ERR_NONE    0   /* No error */
#define BOT_ERR_FMT     1   /* Incorrect message format, does not happen if player uses this library */
#define BOT_ERR_SYSCALL 2   /* Failure during system calls */
#define BOT_ERR_TOOLONG 3   /* Message too long */
#define BOT_ERR_CLOSED  4   /* Pipe closed, usually caused by program exiting */
#define BOT_ERR_TIMEOUT 5   /* Time out */

const char *bot_strerr(int code);

/* Judge side interfaces */

/* Initializes players from command line arguments */
int bot_judge_init(int argc, char *const argv[]);

/* Terminates the player and flushes all output */
void bot_judge_finish();

/* Sends to and receives from a player */
void bot_judge_send(int id, const char *str);

/*
  Returns a string on success, and NULL on failure.
  The returned string, if non-null, should be free()'d.
  `o_len` holds the length on success, or an error code on failure.
  See constant definitions at top of the file, or use `bot_strerr()`
  to get the error description.
 */
char *bot_judge_recv(int id, int *o_len, int timeout);

/* Player side interfaces */

/* Sends to stdout and receives from stdin */
void bot_send(const char *s);

/* Receives a string. The returned string should be free()'d. */
char *bot_recv();

#ifdef __cplusplus
}
#endif

#endif

#ifdef _WIN32
#include <io.h>
#include <process.h>
#include <windows.h>
#pragma warning(disable : 4996)
/*
  Deprecated POSIX names:
    write, open
 */
#undef STDIN_FILENO
#undef STDOUT_FILENO
#undef STDERR_FILENO
#define STDIN_FILENO  _fileno(stdin)
#define STDOUT_FILENO _fileno(stdout)
#define STDERR_FILENO _fileno(stderr)
#else
#include <poll.h>
#include <sys/wait.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define quq(__syscall, ...) _quq(#__syscall, __syscall(__VA_ARGS__))

static int _quq(const char *name, int ret)
{
    if (ret == -1) {
        fprintf(stderr, "%s() failed > < [errno %d]\n", name, errno);
        exit(1);
    }
    return ret;
}

static inline long diff_ms(const struct timespec t1, const struct timespec t2)
{
    long ret = (t2.tv_sec - t1.tv_sec) * 1000;
    ret += (1000000000 + t2.tv_nsec - t1.tv_nsec) / 1000000 - 1000;
    return ret;
}

static inline char tenacious_write(int fd, const char *buf, size_t len)
{
    size_t ptr = 0;
    while (ptr < len) {
        int written = write(fd, buf + ptr, len - ptr);
        if (written == -1) return -1;
        ptr += written;
    }
    return 0;
}

/* Sends data prefixed with its 24-bit length to a file descriptor. */
static int bot_send_blob(int pipe, size_t len, const char *payload)
{
    if (len == 0) len = strlen(payload);
    if (len > 0xffffff) return BOT_ERR_TOOLONG;
    char len_buf[3] = {
        (char)(len & 0xff),
        (char)((len >> 8) & 0xff),
        (char)((len >> 16) & 0xff)
    };
    if (tenacious_write(pipe, len_buf, 3) < 0 ||
        tenacious_write(pipe, payload, len) < 0)
    {
        fprintf(stderr, "write() failed with errno %d\n", errno);
        return BOT_ERR_SYSCALL;
    }
    return BOT_ERR_NONE;
}

/*
  Receives data from a file descriptor with a given timeout.
  Returns a pointer to the data and stores the length in *o_len.
  In case of errors, returns NULL, stores the error code (see above) in *o_len,
  and prints related messages to stderr.
 */
static char *bot_recv_blob(int pipe, size_t *o_len, int timeout)
{
#ifdef _WIN32
    /* Under windows, a simple implementation without timeout is employed */
    unsigned char buf[4];
    if (_read(pipe, buf, 3) < 3) {
        *o_len = BOT_ERR_FMT;
        return NULL;
    }
    int len = ((unsigned int)buf[2] << 16) |
        ((unsigned int)buf[1] << 8) |
        (unsigned int)buf[0];

    char *ret = (char *)malloc(len + 1);
    int ptr = 0;

    while (ptr < len) {
        int read_len = _read(pipe, ret + ptr, len - ptr);
        if (read_len == -1) {
            free(ret);
            *o_len = BOT_ERR_SYSCALL;
            return NULL;
        }
        ptr += read_len;
    }

#else
    struct pollfd pfd = (struct pollfd){pipe, POLLIN, 0};
    char *ret = NULL;
    size_t len = 0, ptr = 0;
    unsigned char buf[4];
    struct timespec t1, t2;
    int loops = 0;

    while (len == 0 || ptr < len) {
        /* Unlikely; in case poll()'s time slightly differs
           from CLOCK_MONOTONIC, or rounding errors happen */
        if ((timeout <= 0 || ++loops >= 1000000) && timeout != -1) {
            fprintf(stderr, "Unidentifiable exception with errno %d\n", errno);
            if (ret) free(ret);
            *o_len = BOT_ERR_SYSCALL;
            return NULL;
        }

        /* Keep time */
        clock_gettime(CLOCK_MONOTONIC, &t1);
        /* Wait for reading */
        pfd.revents = 0;
        int poll_ret = poll(&pfd, 1, timeout);
        if (poll_ret == -1) {
            fprintf(stderr, "poll() failed with errno %d\n", errno);
            if (ret) free(ret);
            *o_len = BOT_ERR_SYSCALL;
            return NULL;
        }
        /* Calculate remaining time */
        clock_gettime(CLOCK_MONOTONIC, &t2);
        if (timeout > 0) timeout -= diff_ms(t1, t2);

        /* Is the pipe still open on the other side? */
        if (pfd.revents & POLLHUP) {
            if (ret) free(ret);
            *o_len = BOT_ERR_CLOSED;
            return NULL;
        }

        if (poll_ret == 1 && (pfd.revents & POLLIN)) {
            /* Ready for reading! Let's see */
            int read_len;

            if (ret == NULL) {
                read_len = read(pipe, buf, 3);
            } else {
                read_len = read(pipe, ret + ptr, len - ptr);
            }

            if (read_len == -1) {
                fprintf(stderr, "read() failed with errno %d\n", errno);
                if (ret) free(ret);
                *o_len = BOT_ERR_SYSCALL;
                return NULL;
            }

            if (ret == NULL) {
                if (read_len < 3) {
                    /* Invalid */
                    if (ret) free(ret);
                    *o_len = BOT_ERR_FMT;
                    return NULL;
                }
                /* Parse the length */
                len = ((unsigned int)buf[2] << 16) |
                    ((unsigned int)buf[1] << 8) |
                    (unsigned int)buf[0];
                ret = (char *)malloc(len + 1);
                if (len == 0) break;    /* Nothing to read */
            } else {
                /* Move buffer pointer */
                ptr += read_len;
            }
        } else {
            if (ret) free(ret);
            if (timeout <= 0) {
                *o_len = BOT_ERR_TIMEOUT;
            } else {
                fprintf(stderr, "poll() returns unexpected events %d\n", pfd.revents);
                *o_len = BOT_ERR_SYSCALL;
            }
            return NULL;
        }
    }
#endif

    *o_len = len;
    ret[len] = '\0';
    return ret;
}

const char *bot_strerr(int code)
{
    switch (code) {
        case BOT_ERR_NONE:    return "ok";
        case BOT_ERR_FMT:     return "incorrect message format";
        case BOT_ERR_SYSCALL: return "failure during system calls";
        case BOT_ERR_TOOLONG: return "message too long";
        case BOT_ERR_CLOSED:  return "pipe closed";
        case BOT_ERR_TIMEOUT: return "timeout";
        default:              return "unknown error";
    }
}

typedef struct _bot_player {
#ifdef _WIN32
    intptr_t pid;
#else
    pid_t pid;
#endif

    /* fd_send is the child's stdin, fd_recv is stdout
       Parent writes to fd_send and reads from fd_recv */
    int fd_send, fd_recv;
    int fd_log;
} bot_player;

#ifdef _WIN32
#define bot_judge_pause(__cp)
#define bot_judge_resume(__cp)
#else
#define bot_judge_pause(__cp)   kill(-(__cp).pid, SIGUSR1)
#define bot_judge_resume(__cp)  kill(-(__cp).pid, SIGUSR2)
#endif

/*
  Creates the child.
  Child processes are normally paused, but during `bot_judge_recv()`
  the process is resumed, and paused again after its response arrives.
 */
static bot_player bot_judge_create(const char *cmd, const char *log)
{
    bot_player ret;
    ret.pid = -1;

    int fd_send[2], fd_recv[2];
#ifdef _WIN32
    if (_pipe(fd_send, 1024, _O_BINARY) != 0 ||
        _pipe(fd_recv, 1024, _O_BINARY) != 0)
#else
    if (pipe(fd_send) != 0 || pipe(fd_recv) != 0)
#endif
    {
        fprintf(stderr, "pipe() failed with errno %d\n", errno);
        exit(1);    /* Non-zero exit status by the judge will be
                       reported as "System Error" */
    }

    int fd_log = open(log, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_log == -1) {
        fprintf(stderr, "open(%s) failed with errno %d\n", log, errno);
        exit(1);
    }

#ifdef _WIN32
    /* Back up stdin/stdout/stderr, since following _dup2() calls
       overwrite these */
    int fd_stdin = _dup(STDIN_FILENO);
    int fd_stdout = _dup(STDOUT_FILENO);
    int fd_stderr = _dup(STDERR_FILENO);

    _dup2(fd_send[0], STDIN_FILENO);
    _dup2(fd_recv[1], STDOUT_FILENO);
    _dup2(fd_log, STDERR_FILENO);

    /* Spawn the child process */
    intptr_t handle = _spawnl(_P_NOWAIT, cmd, cmd, NULL);

    ret.pid = handle;
    ret.fd_log = fd_log;

    /* Restore original stdin/stdout/stderr */
    _dup2(fd_stdin, STDIN_FILENO);
    _dup2(fd_stdout, STDOUT_FILENO);
    _dup2(fd_stderr, STDERR_FILENO);
    _close(fd_stdin);
    _close(fd_stdout);

#else
    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "fork() failed with errno %d\n", errno);
        exit(1);
    }

    if (pid == 0) {
        /* Child process */
        dup2(fd_send[0], STDIN_FILENO);
        dup2(fd_recv[1], STDOUT_FILENO);
        dup2(fd_log, STDERR_FILENO);
        close(fd_send[1]);
        close(fd_recv[0]);
        if (execl(cmd, cmd, (char *)NULL) != 0) {
            if (execl("/bin/sh", "/bin/sh", "-c", cmd, (char *)NULL) != 0) {
                fprintf(stderr, "exec(%s) failed with errno %d\n", cmd, errno);
                exit(1);
            }
        }
    }

    ret.pid = pid;
#endif

    /* Parent process */
    close(fd_send[0]);
    close(fd_recv[1]);
    ret.fd_send = fd_send[1];
    ret.fd_recv = fd_recv[0];
    bot_judge_pause(ret);

    return ret;
}

int num_players;
static bot_player *players;

int bot_judge_init(int argc, char *const argv[])
{
    int n = (argc - 1) / 2;
    num_players = n;
    players = (bot_player *)malloc(sizeof(bot_player) * n);

    int i;
    for (i = 0; i < n; i++)
        players[i] = bot_judge_create(argv[1 + i], argv[1 + i + n]);

    return n;
}

void bot_judge_finish()
{
    int i;
    for (i = 0; i < num_players; i++) {
#ifdef _WIN32
        _close(players[i].fd_log);
        TerminateProcess((HANDLE)players[i].pid, 0);
#else
        fsync(players[i].fd_log);
        kill(players[i].pid, SIGTERM);
        while (waitpid(players[i].pid, 0, 0) > 0) { }
#endif
    }
}

void bot_judge_send(int id, const char *str)
{
    bot_send_blob(players[id].fd_send, 0, str);
}

char *bot_judge_recv(int id, int *o_len, int timeout)
{
    size_t len;
    bot_judge_resume(players[id]);
    char *resp = bot_recv_blob(players[id].fd_recv, &len, timeout);
    bot_judge_pause(players[id]);
    if (o_len != NULL) *o_len = len;
    return resp;
}

void bot_send(const char *s)
{
    bot_send_blob(STDOUT_FILENO, 0, s);
}

char *bot_recv()
{
    size_t len;
    char *ret = bot_recv_blob(STDIN_FILENO, &len, -1);
    return ret;
}

#define _CRT_SECURE_NO_WARNINGS
#ifndef GAME_H
#define GAME_H

#include "Map.h"
#include "Player.h"

#ifndef JSON_CONFIG_H_INCLUDED
# define JSON_CONFIG_H_INCLUDED

/// If defined, indicates that json library is embedded in CppTL library.
//# define JSON_IN_CPPTL 1

/// If defined, indicates that json may leverage CppTL library
//#  define JSON_USE_CPPTL 1
/// If defined, indicates that cpptl vector based map should be used instead of std::map
/// as Value container.
//#  define JSON_USE_CPPTL_SMALLMAP 1
/// If defined, indicates that Json specific container should be used
/// (hash table & simple deque container with customizable allocator).
/// THIS FEATURE IS STILL EXPERIMENTAL!
//#  define JSON_VALUE_USE_INTERNAL_MAP 1
/// Force usage of standard new/malloc based allocator instead of memory pool based allocator.
/// The memory pools allocator used optimization (initializing Value and ValueInternalLink
/// as if it was a POD) that may cause some validation tool to report errors.
/// Only has effects if JSON_VALUE_USE_INTERNAL_MAP is defined.
//#  define JSON_USE_SIMPLE_INTERNAL_ALLOCATOR 1

/// If defined, indicates that Json use exception to report invalid type manipulation
/// instead of C assert macro.
# define JSON_USE_EXCEPTION 1

# ifdef JSON_IN_CPPTL
#  include <cpptl/config.h>
#  ifndef JSON_USE_CPPTL
#   define JSON_USE_CPPTL 1
#  endif
# endif

# ifdef JSON_IN_CPPTL
#  define JSON_API CPPTL_API
# elif defined(JSON_DLL_BUILD)
#  define JSON_API __declspec(dllexport)
# elif defined(JSON_DLL)
#  define JSON_API __declspec(dllimport)
# else
#  define JSON_API
# endif

#endif // JSON_CONFIG_H_INCLUDED

#ifndef JSON_FORWARDS_H_INCLUDED
# define JSON_FORWARDS_H_INCLUDED


namespace Json {

   // writer.h
   class FastWriter;
   class StyledWriter;

   // reader.h
   class Reader;

   // features.h
   class Features;

   // value.h
   typedef int Int;
   typedef unsigned int UInt;
   class StaticString;
   class Path;
   class PathArgument;
   class Value;
   class ValueIteratorBase;
   class ValueIterator;
   class ValueConstIterator;
#ifdef JSON_VALUE_USE_INTERNAL_MAP
   class ValueAllocator;
   class ValueMapAllocator;
   class ValueInternalLink;
   class ValueInternalArray;
   class ValueInternalMap;
#endif // #ifdef JSON_VALUE_USE_INTERNAL_MAP

} // namespace Json


#endif // JSON_FORWARDS_H_INCLUDED

#ifndef JSON_AUTOLINK_H_INCLUDED
# define JSON_AUTOLINK_H_INCLUDED


# ifdef JSON_IN_CPPTL
#  include <cpptl/cpptl_autolink.h>
# endif

# if !defined(JSON_NO_AUTOLINK)  &&  !defined(JSON_DLL_BUILD)  &&  !defined(JSON_IN_CPPTL)
#  define CPPTL_AUTOLINK_NAME "json"
#  undef CPPTL_AUTOLINK_DLL
#  ifdef JSON_DLL
#   define CPPTL_AUTOLINK_DLL
#  endif
# endif

#endif // JSON_AUTOLINK_H_INCLUDED

#ifndef CPPTL_JSON_FEATURES_H_INCLUDED
# define CPPTL_JSON_FEATURES_H_INCLUDED

namespace Json {

   /** \brief Configuration passed to reader and writer.
    * This configuration object can be used to force the Reader or Writer
    * to behave in a standard conforming way.
    */
   class JSON_API Features
   {
   public:
      /** \brief A configuration that allows all features and assumes all strings are UTF-8.
       * - C & C++ comments are allowed
       * - Root object can be any JSON value
       * - Assumes Value strings are encoded in UTF-8
       */
      static Features all();

      /** \brief A configuration that is strictly compatible with the JSON specification.
       * - Comments are forbidden.
       * - Root object must be either an array or an object value.
       * - Assumes Value strings are encoded in UTF-8
       */
      static Features strictMode();

      /** \brief Initialize the configuration like JsonConfig::allFeatures;
       */
      Features();

      /// \c true if comments are allowed. Default: \c true.
      bool allowComments_;

      /// \c true if root must be either an array or an object value. Default: \c false.
      bool strictRoot_;
   };

} // namespace Json

#endif // CPPTL_JSON_FEATURES_H_INCLUDED

#ifndef CPPTL_JSON_H_INCLUDED
# define CPPTL_JSON_H_INCLUDED

# include <string>
# include <vector>

# ifndef JSON_USE_CPPTL_SMALLMAP
#  include <map>
# else
#  include <cpptl/smallmap.h>
# endif
# ifdef JSON_USE_CPPTL
#  include <cpptl/forwards.h>
# endif

/** \brief JSON (JavaScript Object Notation).
 */
namespace Json {

   /** \brief Type of the value held by a Value object.
    */
   enum ValueType
   {
      nullValue = 0, ///< 'null' value
      intValue,      ///< signed integer value
      uintValue,     ///< unsigned integer value
      realValue,     ///< double value
      stringValue,   ///< UTF-8 string value
      booleanValue,  ///< bool value
      arrayValue,    ///< array value (ordered list)
      objectValue    ///< object value (collection of name/value pairs).
   };

   enum CommentPlacement
   {
      commentBefore = 0,        ///< a comment placed on the line before a value
      commentAfterOnSameLine,   ///< a comment just after a value on the same line
      commentAfter,             ///< a comment on the line after a value (only make sense for root value)
      numberOfCommentPlacement
   };

//# ifdef JSON_USE_CPPTL
//   typedef CppTL::AnyEnumerator<const char *> EnumMemberNames;
//   typedef CppTL::AnyEnumerator<const Value &> EnumValues;
//# endif

   /** \brief Lightweight wrapper to tag static string.
    *
    * Value constructor and objectValue member assignement takes advantage of the
    * StaticString and avoid the cost of string duplication when storing the
    * string or the member name.
    *
    * Example of usage:
    * \code
    * Json::Value aValue( StaticString("some text") );
    * Json::Value object;
    * static const StaticString code("code");
    * object[code] = 1234;
    * \endcode
    */
   class JSON_API StaticString
   {
   public:
      explicit StaticString( const char *czstring )
         : str_( czstring )
      {
      }

      operator const char *() const
      {
         return str_;
      }

      const char *c_str() const
      {
         return str_;
      }

   private:
      const char *str_;
   };

   /** \brief Represents a <a HREF="http://www.json.org">JSON</a> value.
    *
    * This class is a discriminated union wrapper that can represents a:
    * - signed integer [range: Value::minInt - Value::maxInt]
    * - unsigned integer (range: 0 - Value::maxUInt)
    * - double
    * - UTF-8 string
    * - boolean
    * - 'null'
    * - an ordered list of Value
    * - collection of name/value pairs (javascript object)
    *
    * The type of the held value is represented by a #ValueType and 
    * can be obtained using type().
    *
    * values of an #objectValue or #arrayValue can be accessed using operator[]() methods. 
    * Non const methods will automatically create the a #nullValue element 
    * if it does not exist. 
    * The sequence of an #arrayValue will be automatically resize and initialized 
    * with #nullValue. resize() can be used to enlarge or truncate an #arrayValue.
    *
    * The get() methods can be used to obtanis default value in the case the required element
    * does not exist.
    *
    * It is possible to iterate over the list of a #objectValue values using 
    * the getMemberNames() method.
    */
   class JSON_API Value 
   {
      friend class ValueIteratorBase;
# ifdef JSON_VALUE_USE_INTERNAL_MAP
      friend class ValueInternalLink;
      friend class ValueInternalMap;
# endif
   public:
      typedef std::vector<std::string> Members;
      typedef ValueIterator iterator;
      typedef ValueConstIterator const_iterator;
      typedef Json::UInt UInt;
      typedef Json::Int Int;
      typedef UInt ArrayIndex;

      static const Value null;
      static const Int minInt;
      static const Int maxInt;
      static const UInt maxUInt;

   private:
#ifndef JSONCPP_DOC_EXCLUDE_IMPLEMENTATION
# ifndef JSON_VALUE_USE_INTERNAL_MAP
      class CZString 
      {
      public:
         enum DuplicationPolicy 
         {
            noDuplication = 0,
            duplicate,
            duplicateOnCopy
         };
         CZString( int index );
         CZString( const char *cstr, DuplicationPolicy allocate );
         CZString( const CZString &other );
         ~CZString();
         CZString &operator =( const CZString &other );
         bool operator<( const CZString &other ) const;
         bool operator==( const CZString &other ) const;
         int index() const;
         const char *c_str() const;
         bool isStaticString() const;
      private:
         void swap( CZString &other );
         const char *cstr_;
         int index_;
      };

   public:
#  ifndef JSON_USE_CPPTL_SMALLMAP
      typedef std::map<CZString, Value> ObjectValues;
#  else
      typedef CppTL::SmallMap<CZString, Value> ObjectValues;
#  endif // ifndef JSON_USE_CPPTL_SMALLMAP
# endif // ifndef JSON_VALUE_USE_INTERNAL_MAP
#endif // ifndef JSONCPP_DOC_EXCLUDE_IMPLEMENTATION

   public:
      /** \brief Create a default Value of the given type.

        This is a very useful constructor.
        To create an empty array, pass arrayValue.
        To create an empty object, pass objectValue.
        Another Value can then be set to this one by assignment.
	This is useful since clear() and resize() will not alter types.

        Examples:
	\code
	Json::Value null_value; // null
	Json::Value arr_value(Json::arrayValue); // []
	Json::Value obj_value(Json::objectValue); // {}
	\endcode
      */
      Value( ValueType type = nullValue );
      Value( Int value );
      Value( UInt value );
      Value( double value );
      Value( const char *value );
      Value( const char *beginValue, const char *endValue );
      /** \brief Constructs a value from a static string.

       * Like other value string constructor but do not duplicate the string for
       * internal storage. The given string must remain alive after the call to this
       * constructor.
       * Example of usage:
       * \code
       * Json::Value aValue( StaticString("some text") );
       * \endcode
       */
      Value( const StaticString &value );
      Value( const std::string &value );
# ifdef JSON_USE_CPPTL
      Value( const CppTL::ConstString &value );
# endif
      Value( bool value );
      Value( const Value &other );
      ~Value();

      Value &operator=( const Value &other );
      /// Swap values.
      /// \note Currently, comments are intentionally not swapped, for
      /// both logic and efficiency.
      void swap( Value &other );

      ValueType type() const;

      bool operator <( const Value &other ) const;
      bool operator <=( const Value &other ) const;
      bool operator >=( const Value &other ) const;
      bool operator >( const Value &other ) const;

      bool operator ==( const Value &other ) const;
      bool operator !=( const Value &other ) const;

      int compare( const Value &other );

      const char *asCString() const;
      std::string asString() const;
# ifdef JSON_USE_CPPTL
      CppTL::ConstString asConstString() const;
# endif
      Int asInt() const;
      UInt asUInt() const;
      double asDouble() const;
      bool asBool() const;

      bool isNull() const;
      bool isBool() const;
      bool isInt() const;
      bool isUInt() const;
      bool isIntegral() const;
      bool isDouble() const;
      bool isNumeric() const;
      bool isString() const;
      bool isArray() const;
      bool isObject() const;

      bool isConvertibleTo( ValueType other ) const;

      /// Number of values in array or object
      UInt size() const;

      /// \brief Return true if empty array, empty object, or null;
      /// otherwise, false.
      bool empty() const;

      /// Return isNull()
      bool operator!() const;

      /// Remove all object members and array elements.
      /// \pre type() is arrayValue, objectValue, or nullValue
      /// \post type() is unchanged
      void clear();

      /// Resize the array to size elements. 
      /// New elements are initialized to null.
      /// May only be called on nullValue or arrayValue.
      /// \pre type() is arrayValue or nullValue
      /// \post type() is arrayValue
      void resize( UInt size );

      /// Access an array element (zero based index ).
      /// If the array contains less than index element, then null value are inserted
      /// in the array so that its size is index+1.
      /// (You may need to say 'value[0u]' to get your compiler to distinguish
      ///  this from the operator[] which takes a string.)
      Value &operator[]( UInt index );
      /// Access an array element (zero based index )
      /// (You may need to say 'value[0u]' to get your compiler to distinguish
      ///  this from the operator[] which takes a string.)
      const Value &operator[]( UInt index ) const;
      /// If the array contains at least index+1 elements, returns the element value, 
      /// otherwise returns defaultValue.
      Value get( UInt index, 
                 const Value &defaultValue ) const;
      /// Return true if index < size().
      bool isValidIndex( UInt index ) const;
      /// \brief Append value to array at the end.
      ///
      /// Equivalent to jsonvalue[jsonvalue.size()] = value;
      Value &append( const Value &value );

      /// Access an object value by name, create a null member if it does not exist.
      Value &operator[]( const char *key );
      /// Access an object value by name, returns null if there is no member with that name.
      const Value &operator[]( const char *key ) const;
      /// Access an object value by name, create a null member if it does not exist.
      Value &operator[]( const std::string &key );
      /// Access an object value by name, returns null if there is no member with that name.
      const Value &operator[]( const std::string &key ) const;
      /** \brief Access an object value by name, create a null member if it does not exist.

       * If the object as no entry for that name, then the member name used to store
       * the new entry is not duplicated.
       * Example of use:
       * \code
       * Json::Value object;
       * static const StaticString code("code");
       * object[code] = 1234;
       * \endcode
       */
      Value &operator[]( const StaticString &key );
# ifdef JSON_USE_CPPTL
      /// Access an object value by name, create a null member if it does not exist.
      Value &operator[]( const CppTL::ConstString &key );
      /// Access an object value by name, returns null if there is no member with that name.
      const Value &operator[]( const CppTL::ConstString &key ) const;
# endif
      /// Return the member named key if it exist, defaultValue otherwise.
      Value get( const char *key, 
                 const Value &defaultValue ) const;
      /// Return the member named key if it exist, defaultValue otherwise.
      Value get( const std::string &key,
                 const Value &defaultValue ) const;
# ifdef JSON_USE_CPPTL
      /// Return the member named key if it exist, defaultValue otherwise.
      Value get( const CppTL::ConstString &key,
                 const Value &defaultValue ) const;
# endif
      /// \brief Remove and return the named member.  
      ///
      /// Do nothing if it did not exist.
      /// \return the removed Value, or null.
      /// \pre type() is objectValue or nullValue
      /// \post type() is unchanged
      Value removeMember( const char* key );
      /// Same as removeMember(const char*)
      Value removeMember( const std::string &key );

      /// Return true if the object has a member named key.
      bool isMember( const char *key ) const;
      /// Return true if the object has a member named key.
      bool isMember( const std::string &key ) const;
# ifdef JSON_USE_CPPTL
      /// Return true if the object has a member named key.
      bool isMember( const CppTL::ConstString &key ) const;
# endif

      /// \brief Return a list of the member names.
      ///
      /// If null, return an empty list.
      /// \pre type() is objectValue or nullValue
      /// \post if type() was nullValue, it remains nullValue
      Members getMemberNames() const;

//# ifdef JSON_USE_CPPTL
//      EnumMemberNames enumMemberNames() const;
//      EnumValues enumValues() const;
//# endif

      /// Comments must be //... or /* ... */
      void setComment( const char *comment,
                       CommentPlacement placement );
      /// Comments must be //... or /* ... */
      void setComment( const std::string &comment,
                       CommentPlacement placement );
      bool hasComment( CommentPlacement placement ) const;
      /// Include delimiters and embedded newlines.
      std::string getComment( CommentPlacement placement ) const;

      std::string toStyledString() const;

      const_iterator begin() const;
      const_iterator end() const;

      iterator begin();
      iterator end();

   private:
      Value &resolveReference( const char *key, 
                               bool isStatic );

# ifdef JSON_VALUE_USE_INTERNAL_MAP
      inline bool isItemAvailable() const
      {
         return itemIsUsed_ == 0;
      }

      inline void setItemUsed( bool isUsed = true )
      {
         itemIsUsed_ = isUsed ? 1 : 0;
      }

      inline bool isMemberNameStatic() const
      {
         return memberNameIsStatic_ == 0;
      }

      inline void setMemberNameIsStatic( bool isStatic )
      {
         memberNameIsStatic_ = isStatic ? 1 : 0;
      }
# endif // # ifdef JSON_VALUE_USE_INTERNAL_MAP

   private:
      struct CommentInfo
      {
         CommentInfo();
         ~CommentInfo();

         void setComment( const char *text );

         char *comment_;
      };

      //struct MemberNamesTransform
      //{
      //   typedef const char *result_type;
      //   const char *operator()( const CZString &name ) const
      //   {
      //      return name.c_str();
      //   }
      //};

      union ValueHolder
      {
         Int int_;
         UInt uint_;
         double real_;
         bool bool_;
         char *string_;
# ifdef JSON_VALUE_USE_INTERNAL_MAP
         ValueInternalArray *array_;
         ValueInternalMap *map_;
#else
         ObjectValues *map_;
# endif
      } value_;
      ValueType type_ : 8;
      int allocated_ : 1;     // Notes: if declared as bool, bitfield is useless.
# ifdef JSON_VALUE_USE_INTERNAL_MAP
      unsigned int itemIsUsed_ : 1;      // used by the ValueInternalMap container.
      int memberNameIsStatic_ : 1;       // used by the ValueInternalMap container.
# endif
      CommentInfo *comments_;
   };


   /** \brief Experimental and untested: represents an element of the "path" to access a node.
    */
   class PathArgument
   {
   public:
      friend class Path;

      PathArgument();
      PathArgument( UInt index );
      PathArgument( const char *key );
      PathArgument( const std::string &key );

   private:
      enum Kind
      {
         kindNone = 0,
         kindIndex,
         kindKey
      };
      std::string key_;
      UInt index_;
      Kind kind_;
   };

   /** \brief Experimental and untested: represents a "path" to access a node.
    *
    * Syntax:
    * - "." => root node
    * - ".[n]" => elements at index 'n' of root node (an array value)
    * - ".name" => member named 'name' of root node (an object value)
    * - ".name1.name2.name3"
    * - ".[0][1][2].name1[3]"
    * - ".%" => member name is provided as parameter
    * - ".[%]" => index is provied as parameter
    */
   class Path
   {
   public:
      Path( const std::string &path,
            const PathArgument &a1 = PathArgument(),
            const PathArgument &a2 = PathArgument(),
            const PathArgument &a3 = PathArgument(),
            const PathArgument &a4 = PathArgument(),
            const PathArgument &a5 = PathArgument() );

      const Value &resolve( const Value &root ) const;
      Value resolve( const Value &root, 
                     const Value &defaultValue ) const;
      /// Creates the "path" to access the specified node and returns a reference on the node.
      Value &make( Value &root ) const;

   private:
      typedef std::vector<const PathArgument *> InArgs;
      typedef std::vector<PathArgument> Args;

      void makePath( const std::string &path,
                     const InArgs &in );
      void addPathInArg( const std::string &path, 
                         const InArgs &in, 
                         InArgs::const_iterator &itInArg, 
                         PathArgument::Kind kind );
      void invalidPath( const std::string &path, 
                        int location );

      Args args_;
   };

   /** \brief Experimental do not use: Allocator to customize member name and string value memory management done by Value.
    *
    * - makeMemberName() and releaseMemberName() are called to respectively duplicate and
    *   free an Json::objectValue member name.
    * - duplicateStringValue() and releaseStringValue() are called similarly to
    *   duplicate and free a Json::stringValue value.
    */
   class ValueAllocator
   {
   public:
      enum { unknown = (unsigned)-1 };

      virtual ~ValueAllocator();

      virtual char *makeMemberName( const char *memberName ) = 0;
      virtual void releaseMemberName( char *memberName ) = 0;
      virtual char *duplicateStringValue( const char *value, 
                                          unsigned int length = unknown ) = 0;
      virtual void releaseStringValue( char *value ) = 0;
   };

#ifdef JSON_VALUE_USE_INTERNAL_MAP
   /** \brief Allocator to customize Value internal map.
    * Below is an example of a simple implementation (default implementation actually
    * use memory pool for speed).
    * \code
      class DefaultValueMapAllocator : public ValueMapAllocator
      {
      public: // overridden from ValueMapAllocator
         virtual ValueInternalMap *newMap()
         {
            return new ValueInternalMap();
         }

         virtual ValueInternalMap *newMapCopy( const ValueInternalMap &other )
         {
            return new ValueInternalMap( other );
         }

         virtual void destructMap( ValueInternalMap *map )
         {
            delete map;
         }

         virtual ValueInternalLink *allocateMapBuckets( unsigned int size )
         {
            return new ValueInternalLink[size];
         }

         virtual void releaseMapBuckets( ValueInternalLink *links )
         {
            delete [] links;
         }

         virtual ValueInternalLink *allocateMapLink()
         {
            return new ValueInternalLink();
         }

         virtual void releaseMapLink( ValueInternalLink *link )
         {
            delete link;
         }
      };
    * \endcode
    */ 
   class JSON_API ValueMapAllocator
   {
   public:
      virtual ~ValueMapAllocator();
      virtual ValueInternalMap *newMap() = 0;
      virtual ValueInternalMap *newMapCopy( const ValueInternalMap &other ) = 0;
      virtual void destructMap( ValueInternalMap *map ) = 0;
      virtual ValueInternalLink *allocateMapBuckets( unsigned int size ) = 0;
      virtual void releaseMapBuckets( ValueInternalLink *links ) = 0;
      virtual ValueInternalLink *allocateMapLink() = 0;
      virtual void releaseMapLink( ValueInternalLink *link ) = 0;
   };

   /** \brief ValueInternalMap hash-map bucket chain link (for internal use only).
    * \internal previous_ & next_ allows for bidirectional traversal.
    */
   class JSON_API ValueInternalLink
   {
   public:
      enum { itemPerLink = 6 };  // sizeof(ValueInternalLink) = 128 on 32 bits architecture.
      enum InternalFlags { 
         flagAvailable = 0,
         flagUsed = 1
      };

      ValueInternalLink();

      ~ValueInternalLink();

      Value items_[itemPerLink];
      char *keys_[itemPerLink];
      ValueInternalLink *previous_;
      ValueInternalLink *next_;
   };


   /** \brief A linked page based hash-table implementation used internally by Value.
    * \internal ValueInternalMap is a tradional bucket based hash-table, with a linked
    * list in each bucket to handle collision. There is an addional twist in that
    * each node of the collision linked list is a page containing a fixed amount of
    * value. This provides a better compromise between memory usage and speed.
    * 
    * Each bucket is made up of a chained list of ValueInternalLink. The last
    * link of a given bucket can be found in the 'previous_' field of the following bucket.
    * The last link of the last bucket is stored in tailLink_ as it has no following bucket.
    * Only the last link of a bucket may contains 'available' item. The last link always
    * contains at least one element unless is it the bucket one very first link.
    */
   class JSON_API ValueInternalMap
   {
      friend class ValueIteratorBase;
      friend class Value;
   public:
      typedef unsigned int HashKey;
      typedef unsigned int BucketIndex;

# ifndef JSONCPP_DOC_EXCLUDE_IMPLEMENTATION
      struct IteratorState
      {
         IteratorState() 
            : map_(0)
            , link_(0)
            , itemIndex_(0)
            , bucketIndex_(0) 
         {
         }
         ValueInternalMap *map_;
         ValueInternalLink *link_;
         BucketIndex itemIndex_;
         BucketIndex bucketIndex_;
      };
# endif // ifndef JSONCPP_DOC_EXCLUDE_IMPLEMENTATION

      ValueInternalMap();
      ValueInternalMap( const ValueInternalMap &other );
      ValueInternalMap &operator =( const ValueInternalMap &other );
      ~ValueInternalMap();

      void swap( ValueInternalMap &other );

      BucketIndex size() const;

      void clear();

      bool reserveDelta( BucketIndex growth );

      bool reserve( BucketIndex newItemCount );

      const Value *find( const char *key ) const;

      Value *find( const char *key );

      Value &resolveReference( const char *key, 
                               bool isStatic );

      void remove( const char *key );

      void doActualRemove( ValueInternalLink *link, 
                           BucketIndex index,
                           BucketIndex bucketIndex );

      ValueInternalLink *&getLastLinkInBucket( BucketIndex bucketIndex );

      Value &setNewItem( const char *key, 
                         bool isStatic, 
                         ValueInternalLink *link, 
                         BucketIndex index );

      Value &unsafeAdd( const char *key, 
                        bool isStatic, 
                        HashKey hashedKey );

      HashKey hash( const char *key ) const;

      int compare( const ValueInternalMap &other ) const;

   private:
      void makeBeginIterator( IteratorState &it ) const;
      void makeEndIterator( IteratorState &it ) const;
      static bool equals( const IteratorState &x, const IteratorState &other );
      static void increment( IteratorState &iterator );
      static void incrementBucket( IteratorState &iterator );
      static void decrement( IteratorState &iterator );
      static const char *key( const IteratorState &iterator );
      static const char *key( const IteratorState &iterator, bool &isStatic );
      static Value &value( const IteratorState &iterator );
      static int distance( const IteratorState &x, const IteratorState &y );

   private:
      ValueInternalLink *buckets_;
      ValueInternalLink *tailLink_;
      BucketIndex bucketsSize_;
      BucketIndex itemCount_;
   };

   /** \brief A simplified deque implementation used internally by Value.
   * \internal
   * It is based on a list of fixed "page", each page contains a fixed number of items.
   * Instead of using a linked-list, a array of pointer is used for fast item look-up.
   * Look-up for an element is as follow:
   * - compute page index: pageIndex = itemIndex / itemsPerPage
   * - look-up item in page: pages_[pageIndex][itemIndex % itemsPerPage]
   *
   * Insertion is amortized constant time (only the array containing the index of pointers
   * need to be reallocated when items are appended).
   */
   class JSON_API ValueInternalArray
   {
      friend class Value;
      friend class ValueIteratorBase;
   public:
      enum { itemsPerPage = 8 };    // should be a power of 2 for fast divide and modulo.
      typedef Value::ArrayIndex ArrayIndex;
      typedef unsigned int PageIndex;

# ifndef JSONCPP_DOC_EXCLUDE_IMPLEMENTATION
      struct IteratorState // Must be a POD
      {
         IteratorState() 
            : array_(0)
            , currentPageIndex_(0)
            , currentItemIndex_(0) 
         {
         }
         ValueInternalArray *array_;
         Value **currentPageIndex_;
         unsigned int currentItemIndex_;
      };
# endif // ifndef JSONCPP_DOC_EXCLUDE_IMPLEMENTATION

      ValueInternalArray();
      ValueInternalArray( const ValueInternalArray &other );
      ValueInternalArray &operator =( const ValueInternalArray &other );
      ~ValueInternalArray();
      void swap( ValueInternalArray &other );

      void clear();
      void resize( ArrayIndex newSize );

      Value &resolveReference( ArrayIndex index );

      Value *find( ArrayIndex index ) const;

      ArrayIndex size() const;

      int compare( const ValueInternalArray &other ) const;

   private:
      static bool equals( const IteratorState &x, const IteratorState &other );
      static void increment( IteratorState &iterator );
      static void decrement( IteratorState &iterator );
      static Value &dereference( const IteratorState &iterator );
      static Value &unsafeDereference( const IteratorState &iterator );
      static int distance( const IteratorState &x, const IteratorState &y );
      static ArrayIndex indexOf( const IteratorState &iterator );
      void makeBeginIterator( IteratorState &it ) const;
      void makeEndIterator( IteratorState &it ) const;
      void makeIterator( IteratorState &it, ArrayIndex index ) const;

      void makeIndexValid( ArrayIndex index );

      Value **pages_;
      ArrayIndex size_;
      PageIndex pageCount_;
   };

   /** \brief Experimental: do not use. Allocator to customize Value internal array.
    * Below is an example of a simple implementation (actual implementation use
    * memory pool).
      \code
class DefaultValueArrayAllocator : public ValueArrayAllocator
{
public: // overridden from ValueArrayAllocator
   virtual ~DefaultValueArrayAllocator()
   {
   }

   virtual ValueInternalArray *newArray()
   {
      return new ValueInternalArray();
   }

   virtual ValueInternalArray *newArrayCopy( const ValueInternalArray &other )
   {
      return new ValueInternalArray( other );
   }

   virtual void destruct( ValueInternalArray *array )
   {
      delete array;
   }

   virtual void reallocateArrayPageIndex( Value **&indexes, 
                                          ValueInternalArray::PageIndex &indexCount,
                                          ValueInternalArray::PageIndex minNewIndexCount )
   {
      ValueInternalArray::PageIndex newIndexCount = (indexCount*3)/2 + 1;
      if ( minNewIndexCount > newIndexCount )
         newIndexCount = minNewIndexCount;
      void *newIndexes = realloc( indexes, sizeof(Value*) * newIndexCount );
      if ( !newIndexes )
         throw std::bad_alloc();
      indexCount = newIndexCount;
      indexes = static_cast<Value **>( newIndexes );
   }
   virtual void releaseArrayPageIndex( Value **indexes, 
                                       ValueInternalArray::PageIndex indexCount )
   {
      if ( indexes )
         free( indexes );
   }

   virtual Value *allocateArrayPage()
   {
      return static_cast<Value *>( malloc( sizeof(Value) * ValueInternalArray::itemsPerPage ) );
   }

   virtual void releaseArrayPage( Value *value )
   {
      if ( value )
         free( value );
   }
};
      \endcode
    */ 
   class JSON_API ValueArrayAllocator
   {
   public:
      virtual ~ValueArrayAllocator();
      virtual ValueInternalArray *newArray() = 0;
      virtual ValueInternalArray *newArrayCopy( const ValueInternalArray &other ) = 0;
      virtual void destructArray( ValueInternalArray *array ) = 0;
      /** \brief Reallocate array page index.
       * Reallocates an array of pointer on each page.
       * \param indexes [input] pointer on the current index. May be \c NULL.
       *                [output] pointer on the new index of at least 
       *                         \a minNewIndexCount pages. 
       * \param indexCount [input] current number of pages in the index.
       *                   [output] number of page the reallocated index can handle.
       *                            \b MUST be >= \a minNewIndexCount.
       * \param minNewIndexCount Minimum number of page the new index must be able to
       *                         handle.
       */
      virtual void reallocateArrayPageIndex( Value **&indexes, 
                                             ValueInternalArray::PageIndex &indexCount,
                                             ValueInternalArray::PageIndex minNewIndexCount ) = 0;
      virtual void releaseArrayPageIndex( Value **indexes, 
                                          ValueInternalArray::PageIndex indexCount ) = 0;
      virtual Value *allocateArrayPage() = 0;
      virtual void releaseArrayPage( Value *value ) = 0;
   };
#endif // #ifdef JSON_VALUE_USE_INTERNAL_MAP


   /** \brief base class for Value iterators.
    *
    */
   class ValueIteratorBase
   {
   public:
      typedef unsigned int size_t;
      typedef int difference_type;
      typedef ValueIteratorBase SelfType;

      ValueIteratorBase();
#ifndef JSON_VALUE_USE_INTERNAL_MAP
      explicit ValueIteratorBase( const Value::ObjectValues::iterator &current );
#else
      ValueIteratorBase( const ValueInternalArray::IteratorState &state );
      ValueIteratorBase( const ValueInternalMap::IteratorState &state );
#endif

      bool operator ==( const SelfType &other ) const
      {
         return isEqual( other );
      }

      bool operator !=( const SelfType &other ) const
      {
         return !isEqual( other );
      }

      difference_type operator -( const SelfType &other ) const
      {
         return computeDistance( other );
      }

      /// Return either the index or the member name of the referenced value as a Value.
      Value key() const;

      /// Return the index of the referenced Value. -1 if it is not an arrayValue.
      UInt index() const;

      /// Return the member name of the referenced Value. "" if it is not an objectValue.
      const char *memberName() const;

   protected:
      Value &deref() const;

      void increment();

      void decrement();

      difference_type computeDistance( const SelfType &other ) const;

      bool isEqual( const SelfType &other ) const;

      void copy( const SelfType &other );

   private:
#ifndef JSON_VALUE_USE_INTERNAL_MAP
      Value::ObjectValues::iterator current_;
      // Indicates that iterator is for a null value.
      bool isNull_;
#else
      union
      {
         ValueInternalArray::IteratorState array_;
         ValueInternalMap::IteratorState map_;
      } iterator_;
      bool isArray_;
#endif
   };

   /** \brief const iterator for object and array value.
    *
    */
   class ValueConstIterator : public ValueIteratorBase
   {
      friend class Value;
   public:
      typedef unsigned int size_t;
      typedef int difference_type;
      typedef const Value &reference;
      typedef const Value *pointer;
      typedef ValueConstIterator SelfType;

      ValueConstIterator();
   private:
      /*! \internal Use by Value to create an iterator.
       */
#ifndef JSON_VALUE_USE_INTERNAL_MAP
      explicit ValueConstIterator( const Value::ObjectValues::iterator &current );
#else
      ValueConstIterator( const ValueInternalArray::IteratorState &state );
      ValueConstIterator( const ValueInternalMap::IteratorState &state );
#endif
   public:
      SelfType &operator =( const ValueIteratorBase &other );

      SelfType operator++( int )
      {
         SelfType temp( *this );
         ++*this;
         return temp;
      }

      SelfType operator--( int )
      {
         SelfType temp( *this );
         --*this;
         return temp;
      }

      SelfType &operator--()
      {
         decrement();
         return *this;
      }

      SelfType &operator++()
      {
         increment();
         return *this;
      }

      reference operator *() const
      {
         return deref();
      }
   };


   /** \brief Iterator for object and array value.
    */
   class ValueIterator : public ValueIteratorBase
   {
      friend class Value;
   public:
      typedef unsigned int size_t;
      typedef int difference_type;
      typedef Value &reference;
      typedef Value *pointer;
      typedef ValueIterator SelfType;

      ValueIterator();
      ValueIterator( const ValueConstIterator &other );
      ValueIterator( const ValueIterator &other );
   private:
      /*! \internal Use by Value to create an iterator.
       */
#ifndef JSON_VALUE_USE_INTERNAL_MAP
      explicit ValueIterator( const Value::ObjectValues::iterator &current );
#else
      ValueIterator( const ValueInternalArray::IteratorState &state );
      ValueIterator( const ValueInternalMap::IteratorState &state );
#endif
   public:

      SelfType &operator =( const SelfType &other );

      SelfType operator++( int )
      {
         SelfType temp( *this );
         ++*this;
         return temp;
      }

      SelfType operator--( int )
      {
         SelfType temp( *this );
         --*this;
         return temp;
      }

      SelfType &operator--()
      {
         decrement();
         return *this;
      }

      SelfType &operator++()
      {
         increment();
         return *this;
      }

      reference operator *() const
      {
         return deref();
      }
   };


} // namespace Json


#endif // CPPTL_JSON_H_INCLUDED

#ifndef CPPTL_JSON_READER_H_INCLUDED
# define CPPTL_JSON_READER_H_INCLUDED

# include <deque>
# include <stack>
# include <string>
# include <iostream>

namespace Json {

   /** \brief Unserialize a <a HREF="http://www.json.org">JSON</a> document into a Value.
    *
    */
   class JSON_API Reader
   {
   public:
      typedef char Char;
      typedef const Char *Location;

      /** \brief Constructs a Reader allowing all features
       * for parsing.
       */
      Reader();

      /** \brief Constructs a Reader allowing the specified feature set
       * for parsing.
       */
      Reader( const Features &features );

      /** \brief Read a Value from a <a HREF="http://www.json.org">JSON</a> document.
       * \param document UTF-8 encoded string containing the document to read.
       * \param root [out] Contains the root value of the document if it was
       *             successfully parsed.
       * \param collectComments \c true to collect comment and allow writing them back during
       *                        serialization, \c false to discard comments.
       *                        This parameter is ignored if Features::allowComments_
       *                        is \c false.
       * \return \c true if the document was successfully parsed, \c false if an error occurred.
       */
      bool parse( const std::string &document, 
                  Value &root,
                  bool collectComments = true );

      /** \brief Read a Value from a <a HREF="http://www.json.org">JSON</a> document.
       * \param document UTF-8 encoded string containing the document to read.
       * \param root [out] Contains the root value of the document if it was
       *             successfully parsed.
       * \param collectComments \c true to collect comment and allow writing them back during
       *                        serialization, \c false to discard comments.
       *                        This parameter is ignored if Features::allowComments_
       *                        is \c false.
       * \return \c true if the document was successfully parsed, \c false if an error occurred.
       */
      bool parse( const char *beginDoc, const char *endDoc, 
                  Value &root,
                  bool collectComments = true );

      /// \brief Parse from input stream.
      /// \see Json::operator>>(std::istream&, Json::Value&).
      bool parse( std::istream &is,
                  Value &root,
                  bool collectComments = true );

      /** \brief Returns a user friendly string that list errors in the parsed document.
       * \return Formatted error message with the list of errors with their location in 
       *         the parsed document. An empty string is returned if no error occurred
       *         during parsing.
       */
      std::string getFormatedErrorMessages() const;

   private:
      enum TokenType
      {
         tokenEndOfStream = 0,
         tokenObjectBegin,
         tokenObjectEnd,
         tokenArrayBegin,
         tokenArrayEnd,
         tokenString,
         tokenNumber,
         tokenTrue,
         tokenFalse,
         tokenNull,
         tokenArraySeparator,
         tokenMemberSeparator,
         tokenComment,
         tokenError
      };

      class Token
      {
      public:
         TokenType type_;
         Location start_;
         Location end_;
      };

      class ErrorInfo
      {
      public:
         Token token_;
         std::string message_;
         Location extra_;
      };

      typedef std::deque<ErrorInfo> Errors;

      bool expectToken( TokenType type, Token &token, const char *message );
      bool readToken( Token &token );
      void skipSpaces();
      bool match( Location pattern, 
                  int patternLength );
      bool readComment();
      bool readCStyleComment();
      bool readCppStyleComment();
      bool readString();
      void readNumber();
      bool readValue();
      bool readObject( Token &token );
      bool readArray( Token &token );
      bool decodeNumber( Token &token );
      bool decodeString( Token &token );
      bool decodeString( Token &token, std::string &decoded );
      bool decodeDouble( Token &token );
      bool decodeUnicodeCodePoint( Token &token, 
                                   Location &current, 
                                   Location end, 
                                   unsigned int &unicode );
      bool decodeUnicodeEscapeSequence( Token &token, 
                                        Location &current, 
                                        Location end, 
                                        unsigned int &unicode );
      bool addError( const std::string &message, 
                     Token &token,
                     Location extra = 0 );
      bool recoverFromError( TokenType skipUntilToken );
      bool addErrorAndRecover( const std::string &message, 
                               Token &token,
                               TokenType skipUntilToken );
      void skipUntilSpace();
      Value &currentValue();
      Char getNextChar();
      void getLocationLineAndColumn( Location location,
                                     int &line,
                                     int &column ) const;
      std::string getLocationLineAndColumn( Location location ) const;
      void addComment( Location begin, 
                       Location end, 
                       CommentPlacement placement );
      void skipCommentTokens( Token &token );
   
      typedef std::stack<Value *> Nodes;
      Nodes nodes_;
      Errors errors_;
      std::string document_;
      Location begin_;
      Location end_;
      Location current_;
      Location lastValueEnd_;
      Value *lastValue_;
      std::string commentsBefore_;
      Features features_;
      bool collectComments_;
   };

   /** \brief Read from 'sin' into 'root'.

    Always keep comments from the input JSON.

    This can be used to read a file into a particular sub-object.
    For example:
    \code
    Json::Value root;
    cin >> root["dir"]["file"];
    cout << root;
    \endcode
    Result:
    \verbatim
    {
	"dir": {
	    "file": {
		// The input stream JSON would be nested here.
	    }
	}
    }
    \endverbatim
    \throw std::exception on parse error.
    \see Json::operator<<()
   */
   std::istream& operator>>( std::istream&, Value& );

} // namespace Json

#endif // CPPTL_JSON_READER_H_INCLUDED

#ifndef JSON_WRITER_H_INCLUDED
# define JSON_WRITER_H_INCLUDED

# include <vector>
# include <string>
# include <iostream>

namespace Json {

   class Value;

   /** \brief Abstract class for writers.
    */
   class JSON_API Writer
   {
   public:
      virtual ~Writer();

      virtual std::string write( const Value &root ) = 0;
   };

   /** \brief Outputs a Value in <a HREF="http://www.json.org">JSON</a> format without formatting (not human friendly).
    *
    * The JSON document is written in a single line. It is not intended for 'human' consumption,
    * but may be usefull to support feature such as RPC where bandwith is limited.
    * \sa Reader, Value
    */
   class JSON_API FastWriter : public Writer
   {
   public:
      FastWriter();
      virtual ~FastWriter(){}

      void enableYAMLCompatibility();

   public: // overridden from Writer
      virtual std::string write( const Value &root );

   private:
      void writeValue( const Value &value );

      std::string document_;
      bool yamlCompatiblityEnabled_;
   };

   /** \brief Writes a Value in <a HREF="http://www.json.org">JSON</a> format in a human friendly way.
    *
    * The rules for line break and indent are as follow:
    * - Object value:
    *     - if empty then print {} without indent and line break
    *     - if not empty the print '{', line break & indent, print one value per line
    *       and then unindent and line break and print '}'.
    * - Array value:
    *     - if empty then print [] without indent and line break
    *     - if the array contains no object value, empty array or some other value types,
    *       and all the values fit on one lines, then print the array on a single line.
    *     - otherwise, it the values do not fit on one line, or the array contains
    *       object or non empty array, then print one value per line.
    *
    * If the Value have comments then they are outputed according to their #CommentPlacement.
    *
    * \sa Reader, Value, Value::setComment()
    */
   class JSON_API StyledWriter: public Writer
   {
   public:
      StyledWriter();
      virtual ~StyledWriter(){}

   public: // overridden from Writer
      /** \brief Serialize a Value in <a HREF="http://www.json.org">JSON</a> format.
       * \param root Value to serialize.
       * \return String containing the JSON document that represents the root value.
       */
      virtual std::string write( const Value &root );

   private:
      void writeValue( const Value &value );
      void writeArrayValue( const Value &value );
      bool isMultineArray( const Value &value );
      void pushValue( const std::string &value );
      void writeIndent();
      void writeWithIndent( const std::string &value );
      void indent();
      void unindent();
      void writeCommentBeforeValue( const Value &root );
      void writeCommentAfterValueOnSameLine( const Value &root );
      bool hasCommentForValue( const Value &value );
      static std::string normalizeEOL( const std::string &text );

      typedef std::vector<std::string> ChildValues;

      ChildValues childValues_;
      std::string document_;
      std::string indentString_;
      int rightMargin_;
      int indentSize_;
      bool addChildValues_;
   };

   /** \brief Writes a Value in <a HREF="http://www.json.org">JSON</a> format in a human friendly way,
        to a stream rather than to a string.
    *
    * The rules for line break and indent are as follow:
    * - Object value:
    *     - if empty then print {} without indent and line break
    *     - if not empty the print '{', line break & indent, print one value per line
    *       and then unindent and line break and print '}'.
    * - Array value:
    *     - if empty then print [] without indent and line break
    *     - if the array contains no object value, empty array or some other value types,
    *       and all the values fit on one lines, then print the array on a single line.
    *     - otherwise, it the values do not fit on one line, or the array contains
    *       object or non empty array, then print one value per line.
    *
    * If the Value have comments then they are outputed according to their #CommentPlacement.
    *
    * \param indentation Each level will be indented by this amount extra.
    * \sa Reader, Value, Value::setComment()
    */
   class JSON_API StyledStreamWriter
   {
   public:
      StyledStreamWriter( std::string indentation="\t" );
      ~StyledStreamWriter(){}

   public:
      /** \brief Serialize a Value in <a HREF="http://www.json.org">JSON</a> format.
       * \param out Stream to write to. (Can be ostringstream, e.g.)
       * \param root Value to serialize.
       * \note There is no point in deriving from Writer, since write() should not return a value.
       */
      void write( std::ostream &out, const Value &root );

   private:
      void writeValue( const Value &value );
      void writeArrayValue( const Value &value );
      bool isMultineArray( const Value &value );
      void pushValue( const std::string &value );
      void writeIndent();
      void writeWithIndent( const std::string &value );
      void indent();
      void unindent();
      void writeCommentBeforeValue( const Value &root );
      void writeCommentAfterValueOnSameLine( const Value &root );
      bool hasCommentForValue( const Value &value );
      static std::string normalizeEOL( const std::string &text );

      typedef std::vector<std::string> ChildValues;

      ChildValues childValues_;
      std::ostream* document_;
      std::string indentString_;
      int rightMargin_;
      std::string indentation_;
      bool addChildValues_;
   };

   std::string JSON_API valueToString( Int value );
   std::string JSON_API valueToString( UInt value );
   std::string JSON_API valueToString( double value );
   std::string JSON_API valueToString( bool value );
   std::string JSON_API valueToQuotedString( const char *value );

   /// \brief Output using the StyledStreamWriter.
   /// \see Json::operator>>()
   std::ostream& operator<<( std::ostream&, const Value &root );

} // namespace Json



#endif // JSON_WRITER_H_INCLUDED



#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <vector>

const int UPGRADE_COST[6] = { 100, 100, 100, 100, 100, 100 }; // todo：确定升级类型所需要的mine数

class Operation
{
public:
	int type; // 移动(0) 或 攻击(1) 或 none(-1)
	Coordinate target;  // 移动/攻击目标

	bool upgrade; //是否升级
	int upgrade_type; //升级类型：0-移动速度， 1-攻击力， 2-采集速度， 3-回血，4-视野，5-生命值
	Operation() : type(-1), upgrade(false) {};
};



Operation get_operation_red(const Player& player, const Map& map);   // todo : SDK，玩家编写
Operation get_operation_blue(const Player& player, const Map& map);   // todo : SDK，玩家编写
//这样写其实就不用写通信了

Operation get_operation(const Player& player, const Map& map);   // todo : SDK，玩家编写


class Game
{
	const int player_id;

	Player player_red = Player(0, MAP_SIZE - 1, 2 * MAP_SIZE - 2, 0);   
	Player player_blue = Player(1, MAP_SIZE - 1, 0, 2 * MAP_SIZE - 2);

	Map map;
    Map mymap; // 用于player_map()函数中返回值，向选手传参

	/*暂时去掉
	map.enemy_unit.push_back(&player_red);
	map.enemy_unit.push_back(&player_blue);
	map.enemy_num+=2;
	*/
	int turn;
public:
	Json::Value m_root;//用于写json向前端汇报的根节点
	//to do: 更改player初始位置
	// Player ai[2];

	Map buildMap(unsigned seed = 0)		//初始化地图
	{
		Map mymap = Map();
		mymap.mine_num = 0;
		mymap.barrier_num = 0;
		mymap.enemy_num = 0;
		srand(seed);
		for (int i = 0; i < 2 * MAP_SIZE - 1; i++) {
			for (int j = 0; j < 2 * MAP_SIZE - 1; j++) {
				for (int k = 0; k < 2 * MAP_SIZE - 1; k++) {
					if (mymap.isValid(i, j, k))
					{
						//随机判断是否有资源点、障碍物
						int mineidx = -1;
						int barrieridx = -1;
						if (rand() % 10 == 1) { mineidx = mymap.mine_num; }				//十分之一概率， TO DO：修改数值
						else if (rand() % 10 == 2) { barrieridx = mymap.barrier_num; }	//十分之一概率， TO DO：修改数值

						if (mymap.isValid(i, j, k))
						{
							mymap.data[i][j][k] = Point(i, j, k, mineidx, barrieridx, 1);
							mymap.data[i][j][k].isvalid = 1;
							if (mineidx >= 0)
							{
								mymap.mine.push_back(Mine(MINE_NUM, Coordinate(i, j, k)));
								mymap.data[i][j][k].MineIdx = mymap.mine_num;
								mymap.mine_num++;
							}
							if (barrieridx >= 0)
							{
								mymap.barrier.push_back(Coordinate(i, j, k));
								mymap.data[i][j][k].BarrierIdx = mymap.barrier_num;
								mymap.barrier_num++;
							}
						}


					}
				}
			}
		}
		mymap.enemy_unit.push_back(player_red);
		mymap.enemy_unit.push_back(player_blue);
		mymap.data[player_red.pos.x][player_red.pos.y][player_red.pos.z].PlayerIdx = player_red.id;
		mymap.data[player_blue.pos.x][player_blue.pos.y][player_blue.pos.z].PlayerIdx = player_blue.id;
		return mymap;
	}

	const Map& player_map(const Player& player)
	{
		//todo:add
		// map.ememy_unit.clear()
		// if(dist ...) map.enemy_unit.push_back(&player_red);
		// mine ...
		mymap = map;
		mymap.enemy_unit.clear();
		mymap.mine.clear();
		mymap.barrier.clear();
		mymap.enemy_num = 0;
		mymap.mine_num = 0;
		mymap.barrier_num = 0;

		mymap.viewSize = player.sight_range;

		for (int i = 0; i < 2 * MAP_SIZE - 1; i++) {
			for (int j = 0; j < 2 * MAP_SIZE - 1; j++) {
				for (int k = 0; k < 2 * MAP_SIZE - 1; k++) {
					if (mymap.data[i][j][k].isvalid == 1)
					{
						mymap.data[i][j][k].MineIdx = -1;
						mymap.data[i][j][k].BarrierIdx = -1;
						mymap.data[i][j][k].PlayerIdx = -1;
					}
				}
			}
		}

		for (auto i : map.mine)
		{

			if (mymap.getDistance(player.pos, i.pos) <= mymap.viewSize - 1)
			{
				mymap.mine.push_back(i);
				mymap.data[i.pos.x][i.pos.y][i.pos.z].MineIdx = mymap.mine_num++;
			}
		}
		for (auto i : map.barrier)
		{
			if (mymap.getDistance(player.pos, i) <= mymap.viewSize - 1)
			{
				mymap.barrier.push_back(i);
				mymap.data[i.x][i.y][i.z].BarrierIdx = mymap.barrier_num++;
			}
		}
		for (auto i : map.enemy_unit)
		{
			if (mymap.getDistance(player.pos, i.pos) <= mymap.viewSize - 1)
			{
				mymap.enemy_unit.push_back(i);
				mymap.data[i.pos.x][i.pos.y][i.pos.z].PlayerIdx = i.id;
				mymap.enemy_num++;
			}
		}


		return mymap;
	}

	void init_json()			//已在Map的构造函数中实现
	{
		// Barrier
		// Mine
		// 玩家位置！
		//输出json文件
		Json::Value root;
		Json::Value maps;
		for (int i = 0; i < 3; i++)
		{
			maps["size"].append(map.nowSize);
		}
		for (int i = 0; i < map.barrier_num; i++)
		{
			Json::Value tmp_array;
			tmp_array.append(map.barrier[i].x);
			tmp_array.append(map.barrier[i].y);
			tmp_array.append(map.barrier[i].z);
			maps["Barrier"].append(tmp_array);
		}
		for (int i = 0; i < map.mine_num; i++)
		{
			Json::Value tmp_array;
			tmp_array.append(map.mine[i].pos.x);
			tmp_array.append(map.mine[i].pos.y);
			tmp_array.append(map.mine[i].pos.z);
			tmp_array.append(map.mine[i].num);
			maps["Resource"].append(tmp_array);
		}
		root["map"] = maps;
		Json::Value players;
		Json::Value item;
		item["id"] = 0;
		item["name"] = "Player1";
		item["attack_range"] = player_red.attack_range;
		item["sight_range"] = player_red.sight_range;
		item["move_range"] = player_red.move_range;
		item["mine_speed"] = player_red.mine_speed;
		item["atk"] = player_red.at;
		item["hp"] = player_red.hp;
		item["Position"].append(player_red.pos.x);
		item["Position"].append(player_red.pos.y);
		item["Position"].append(player_red.pos.z);
		players.append(item);
		item["id"] = 1;
		item["name"] = "Player2";
		item["attack_range"] = player_blue.attack_range;
		item["sight_range"] = player_blue.sight_range;
		item["move_range"] = player_blue.move_range;
		item["mine_speed"] = player_blue.mine_speed;
		item["atk"] = player_blue.at;
		item["hp"] = player_blue.hp;
		item["Position"].resize(0);
		item["Position"].append(player_blue.pos.x);
		item["Position"].append(player_blue.pos.y);
		item["Position"].append(player_blue.pos.z);
		players.append(item);
		root["players"] = players;
		std::ofstream os;
		os.open("init.json");
		Json::StyledWriter sw;
		os << sw.write(root);
		os.close();
	}
	Json::Value reportEvent(int id, Coordinate pos)
	{
		Json::Value current;
		current["Round"] = turn;
		current["CurrentEvent"] = Json::Value::null;
		current["ActivePlayerId"] = id;
		current["VictimId"].resize(0);
		current["ActivePos"].append(pos.x);
		current["ActivePos"].append(pos.y);
		current["ActivePos"].append(pos.z);
		current["WinnerId"] = Json::Value::null;
		current["exp"] = Json::Value::null;
		for (int i = 0; i < 3; i++)
			current["MapSize"].append(map.nowSize);
		current["MinesLeft"] = Json::Value::null;
		current["UpgradeType"] = Json::Value::null;
		return current;
	}
	//Map limited_map(const Player& p)
	//{
	//    //考虑玩家p的视野
	//    return map;
	//}

	//Map limited_map = Map(map,player_red);		//返回玩家p的视野地图

	Operation regulate(Operation op, const Player& p)
	{
		Operation ret;
		//若不合法，一律返回什么都不做none(-1)

		// 检查type在范围内
		if (op.type < -1 || op.type > 1)
			return ret;

		// 检查target在地图范围内、在攻击/移动距离内
		if (op.type == 0)
		{
            //std::cerr << "into here\n";
			if (!map.isValid(op.target))
            {
                //std::cerr << "invalid target\n";
				return ret;
            }
            //std::cerr << "valid here\n";
			if (map.getDistance(op.target, p.pos) > p.move_range)
				return ret;
            //std::cerr << "still valid here\n";
		}
		else if (op.type == 1)
		{
			if (!map.isValid(op.target))
				return ret;
			if (map.getDistance(op.target, p.pos) > p.attack_range)
				return ret;
		}
		//前10回合攻击保护
		if (op.type == 1)
		{
			if (turn <= 10) // todo : 10?
				return ret;
		}

		//检查移动的终点是否合法
		/*
		if(op.type == 0)
		{
			if(map[op.target].hasBarrier)
				return ret;
			//TODO:检查是否有敌方玩家
		}
		*/

		// 检查升级时机（回合数）是否合法
		// TBD
        //std::cerr << "valid move " << op.type << ' ' << map.getDistance(p.pos,op.target) << "\n";
		return op;
	}

	void upgrade(int type, Player& p)
	{
		switch (type)
		{
		case 0:
			p.move_range++;
			break;
		case 1:
			p.attack_range++;
			break;
		case 2:
			p.mine_speed++;
			break;
		case 3:
			p.hp = (p.hp + 50 > 100 ? 100 : p.hp + 50); //血量上限100
			// TODO: += f(turn) ?
			break;
		case 4:
			p.sight_range++;
			break;
		case 5:
			p.at++;
			break;
		default:
			break;
		}
	}
	Operation get_operation_opponent()
	{
		Operation op;
		char *s = bot_recv();
		sscanf(s, "%d %d %d %d %d %d", &op.type, &op.target.x, &op.target.y, &op.target.z, &op.upgrade, &op.upgrade_type);
		free(s);
		return op;
	}
	void send_operation(Operation op)
	{
		char s[100];
		sprintf(s, "%d %d %d %d %d %d", op.type, op.target.x, op.target.y, op.target.z, op.upgrade, op.upgrade_type);
		bot_send(s);
		return;
	}
	Operation judge_proc(int current_player, int* err)
	{
		char* s = bot_judge_recv(current_player, err, 2000);
		bot_judge_send(current_player^1, s);
		Operation op;
		sscanf(s, "%d %d %d %d %d %d", &op.type, &op.target.x, &op.target.y, &op.target.z, &op.upgrade, &op.upgrade_type);
		free(s);
		return op;
		// TODO: 根据err判断选手程序是否正常返回操作
	}

	bool Update() //进行一个回合：若有一方死亡，游戏结束，返回true，否则返回false
	{
		Operation op;
		int mine_get;

		// update采矿收入
		if (map[player_red.pos].MineIdx != -1)
		{
			if (map.mine[map[player_red.pos].MineIdx].available && map.mine[map[player_red.pos].MineIdx].belong != -1)
			{
				mine_get = map.mine[map[player_red.pos].MineIdx].num;
				if (mine_get > player_red.mine_speed)
					mine_get = player_red.mine_speed;
				player_red.mines += mine_get;
				map.mine[map[player_red.pos].MineIdx].num -= mine_get;

				Json::Value event = reportEvent(0, player_red.pos);
				event["CurrentEvent"] = "GATHER";
				event["exp"] = mine_get;
				event["MinesLeft"] = map.mine[map[player_red.pos].MineIdx].num;
				m_root.append(event);
			}
		}
		if (map[player_blue.pos].MineIdx != -1)
		{
			if (map.mine[map[player_blue.pos].MineIdx].available && map.mine[map[player_blue.pos].MineIdx].belong != 1)
			{
				mine_get = map.mine[map[player_blue.pos].MineIdx].num;
				if (mine_get > player_blue.mine_speed)
					mine_get = player_blue.mine_speed;
				player_blue.mines += mine_get;
				map.mine[map[player_blue.pos].MineIdx].num -= mine_get;
				Json::Value event = reportEvent(1, player_blue.pos);
				event["CurrentEvent"] = "GATHER";
				event["exp"] = mine_get;
				event["MinesLeft"] = map.mine[map[player_blue.pos].MineIdx].num;
				m_root.append(event);
			}
		}

		// get operation from player red(0)
		//op = get_operation_red(player_red, limited_map(player_red));
		// real: 
		//Map(map,player_red);

		//op = get_operation_red(player_red, tmp);
		int err;
		if(player_id == 0)
		{
			op = get_operation(player_red, player_map(player_red)); // 暂时！
			send_operation(op);
		}
		else if(player_id == 1)
			op = get_operation_opponent();
		else // judge -1
		{
			op = judge_proc(0, &err);
			// todo : process err
		}

		op = regulate(op, player_red);

		if (op.type == -1)
		{
			Json::Value event = reportEvent(0, player_red.pos);
			event["CurrentEvent"] = "ERROR";
			m_root.append(event);
		}
		// todo: REPLAY 输出op red



		//更新移动相关（位置）
		if (op.type == 0)
		{
			player_red.pos = op.target;
			Json::Value event = reportEvent(0, player_red.pos);
			event["CurrentEvent"] = "MOVE";
			m_root.append(event);
			for (auto i : map.enemy_unit)
			{
				if (i.id != player_red.id)
				{
					continue;
				}
				else
				{
					i.pos = player_red.pos;
					break;
				}
			}
		}

		//更新攻击相关（血量）
		else if (op.type == 1)
		{
			if (op.target == player_blue.pos)
			{
				player_blue.hp -= player_red.at;
				Json::Value event = reportEvent(0, player_red.pos);
				event["CurrentEvent"] = "ATTACK";
				event["VictimId"].append(player_blue.id);// todo: 由于目前没实现中立单位，故只添加了blue的id
				m_root.append(event);
			}
		}
		//更新升级，注意这里升级是在行动之后（可以改顺序)，应当声明
		if (op.upgrade)
		{
			if (op.upgrade_type >= 0 && op.upgrade_type <= 5) // 检查升级类型是否合法
				if (player_red.mines >= UPGRADE_COST[op.upgrade_type]) // 检查资源是否足够
				{
					upgrade(op.upgrade_type, player_red);
					Json::Value event = reportEvent(0, player_red.pos);
					event["CurrentEvent"] = "UPGRADE";
					switch (op.upgrade_type)
					{
					case 0:
						event["UpgradeType"] = "move_range";
						break;
					case 1:
						event["UpgradeType"] = "attack_range";
						break;
					case 2:
						event["UpgradeType"] = "mine_speed";
						break;
					case 3:
						event["UpgradeType"] = "hp";
						break;
					case 4:
						event["UpgradeType"] = "sight_range";
						break;
					case 5:
						event["UpgradeType"] = "atk";
						break;
					default:
						break;
					}
					m_root.append(event);
				}
			// todo : 调节升级资源数 
		}

		//判断存活状态
		if (player_blue.hp <= 0)
			return true;


		// get operation from player blue(1)
		//real: op = get_operation_blue(player_red, Map(map,player_blue));
		
		if(player_id == 1)
		{
			op = get_operation(player_blue, player_map(player_blue));
			send_operation(op);
		}
		else if(player_id == 0)
			op = get_operation_opponent();
		else // judge -1
		{
			op = judge_proc(1, &err);
			// todo : process err
		}
		
		op = regulate(op, player_blue);

		if (op.type == -1)
		{
			Json::Value event = reportEvent(1, player_blue.pos);
			event["CurrentEvent"] = "ERROR";
			m_root.append(event);
		}
		// todo: REPLAY 输出op blue
		//更新移动相关（位置）
		if (op.type == 0)
		{
            
            //std::cerr << "Moved\n" << op.target.x << ' ' << op.target.y << ' ' << op.target.z << '\n';
			player_blue.pos = op.target;
			Json::Value event = reportEvent(1, player_blue.pos);
			event["CurrentEvent"] = "MOVE";
			m_root.append(event);
			for (auto i : map.enemy_unit)
			{
				if (i.id != player_blue.id)
				{
					continue;
				}
				else
				{
					i.pos = player_blue.pos;
					break;
				}
			}
		}
		//更新攻击相关（血量）
		else if (op.type == 1)
		{
			if (op.target == player_red.pos)
			{
				player_red.hp -= player_blue.at;
				Json::Value event = reportEvent(1, player_blue.pos);
				event["CurrentEvent"] = "ATTACK";
				event["VictimId"].append(player_red.id);// todo: 由于目前没实现中立单位，故只添加了red的id
				m_root.append(event);
			}
		}
		//更新升级，注意这里升级是在行动之后（可以改顺序)，应当声明
		if (op.upgrade)
		{
			if (op.upgrade_type >= 0 && op.upgrade_type <= 5) // 检查升级类型是否合法
				if (player_blue.mines >= UPGRADE_COST[op.upgrade_type]) // 检查资源是否足够
				{
					upgrade(op.upgrade_type, player_blue);
					Json::Value event = reportEvent(1, player_blue.pos);
					event["CurrentEvent"] = "UPGRADE";
					switch (op.upgrade_type)
					{
					case 0:
						event["UpgradeType"] = "move_range";
						break;
					case 1:
						event["UpgradeType"] = "attack_range";
						break;
					case 2:
						event["UpgradeType"] = "mine_speed";
						break;
					case 3:
						event["UpgradeType"] = "hp";
						break;
					case 4:
						event["UpgradeType"] = "sight_range";
						break;
					case 5:
						event["UpgradeType"] = "atk";
						break;
					default:
						break;
					}
					m_root.append(event);
				}
			// todo : 调节升级资源数 
		}

		//判断存活状态
		if (player_red.hp <= 0)
			return true;

		//缩圈
		if (turn >= 0.75 * TURN_COUNT && turn % 5 == 0)
		{
			if (map.nowSize)
				map.nowSize--;
		}
		//缩圈伤害
		if (map.getDistance(player_red.pos, Coordinate(MAP_SIZE-1, MAP_SIZE-1, MAP_SIZE-1)) > map.nowSize)
		{
			player_red.hp -= 10; // TODO: 调整这个值，随时间变化？
		}
		if (map.getDistance(player_blue.pos, Coordinate(MAP_SIZE-1, MAP_SIZE-1, MAP_SIZE-1)) > map.nowSize)
		{
			player_blue.hp -= 10; // TODO: 调整这个值，随时间变化？
		}

		//判断存活状态
		if (player_blue.hp <= 0 || player_red.hp <= 0)
			return true;

		return false;

	}
public:
	Game() : player_id (-2) // -2 stands for uninitialized
	{
		turn = 0; 
		init_json();
		// TODO : replay output
	}
	Game(int x, int y) : player_id(x), map(buildMap(y)){
		turn = 0;
		init_json();
		//TODO : 对于player来说，不需要生成录像（json）
	}

	int proc() // 对局入口，返回胜者编号red(0), blue(1)
	{
		init_json();
		for (turn = 1; turn <= TURN_COUNT; turn++)
		{
			//std::cerr << "turn" << turn << std::endl;
            //std::cerr << player_red.hp << ' '<< player_blue.hp << std::endl;
            //std::cerr << map.getDistance(player_red.pos, Coordinate(MAP_SIZE-1, MAP_SIZE-1, MAP_SIZE-1)) << ' ' << map.getDistance(player_blue.pos, Coordinate(MAP_SIZE-1, MAP_SIZE-1, MAP_SIZE-1)) << ' ' << map.nowSize << std::endl;
			if (Update())
				break;
		}
		//终局结算
		//（todo：伤害结算顺序？如果同时死亡，如何计算？）
		if (player_red.hp <= 0)
		{
			Json::Value event = reportEvent(0, player_red.pos);
			event["CurrentEvent"] = "DIED";
			event["WinnerId"] = player_blue.id;
			m_root.append(event);
			std::cerr << "Game ends in turn " << turn << " : player blue wins!" << std::endl;
			return 1;
		}
		if (player_blue.hp <= 0)
		{
			Json::Value event = reportEvent(1, player_blue.pos);
			event["CurrentEvent"] = "DIED";
			event["WinnerId"] = player_red.id;
			m_root.append(event);
			std::cerr << "Game ends in turn " << turn << " : player red wins!" << std::endl;
			return 0;
		}
		return -1; // 出错，没有死亡
		 // cout << "Game ends in turn " << turn << " : Error, no one die but stop." << endl;
		//输出到REPLAY

	}

};

#endif

#include <stdio.h>

// bot
int bot_judge_init(int argc, char *const argv[]);
void bot_judge_send(int id, const char *str);
char *bot_judge_recv(int id, int *o_len, int timeout);
void bot_judge_finish();

Game *game;

Operation get_operation(const Player& player, const Map& map) {Operation op; return op;}

int main(int argc, char *argv[])
{
    /* Initialize players and make sure there are exactly 2 of them */
    int n = bot_judge_init(argc, argv);
    if (n != 2) {
        fprintf(stderr, "Expected 2 players, got %d\n", n);
        exit(1);
    }

    /* Initialize the game */
    srand(time(0));
    int seed = rand();
    game = new Game(-1,seed);

    /* Inform each player which side it is on and the seed to initialize the map*/
    char s[100];
    sprintf(s, "%d %d", 0, seed);
    bot_judge_send(0, s);
    sprintf(s, "%d %d", 1, seed);
    bot_judge_send(1, s);

    /* Play the game */
    int stat = game->proc();

    /* Write the report to stdout */
    if(stat == 0 || stat == 1)
        printf("The winner is player %d\n", stat);
    else
        printf("Game Ends Abnormally with code %d\n", stat);

    /* Terminate the players */
    bot_judge_finish();

    return 0;
}