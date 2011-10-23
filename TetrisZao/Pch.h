#pragma once
#define  BOOST_ASIO_NO_WIN32_LEAN_AND_MEAN  
#include <boost/asio.hpp>
#include <windows.h>
#include <comdef.h>
#include <d3d9.h>
#include <d3dx9.h>

#include <boost/array.hpp>
#include <boost/assign.hpp>
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>
#include <boost/exception.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/function.hpp>
#include <boost/multi_array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/range.hpp>
#include <boost/random.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/signals.hpp>

typedef boost::uint8_t u8;
typedef boost::uint16_t u16;
typedef boost::uint32_t u32;
typedef boost::uint64_t u64;
typedef boost::int8_t s8;
typedef boost::int16_t s16;
typedef boost::int32_t s32;
typedef boost::int64_t s64;
typedef char c8;

using boost::bind;
using boost::enable_shared_from_this;
using boost::function;
using boost::format;
using boost::noncopyable;
using boost::shared_ptr;
using boost::throw_exception;
using boost::weak_ptr;
using boost::wformat;

namespace asio = boost::asio;
namespace sys = boost::system;
using asio::ip::tcp;

#include <deque>
#include <map>
#include <queue>
#include <vector>

using namespace boost::assign;