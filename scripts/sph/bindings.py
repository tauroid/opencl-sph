'''Wrapper for whereami.h

Generated with:
../../../ctypesgen/ctypesgen.py -o bindings.py -L../../bin -lSPH ../../src/3rdparty/whereami.h ../../src/build_psdata.h ../../src/config.h ../../src/datatypes.h ../../src/macros.h ../../src/note.h ../../src/opencl/clerror.h ../../src/opencl/particle_system_host.h ../../src/opencl/platforminfo.h ../../src/particle_system.h ../../src/stringly.h

Do not modify this file.
'''

__docformat__ =  'restructuredtext'

# Begin preamble

import ctypes, os, sys
from ctypes import *
from numpy.ctypeslib import ndpointer

_int_types = (c_int16, c_int32)
if hasattr(ctypes, 'c_int64'):
    # Some builds of ctypes apparently do not have c_int64
    # defined; it's a pretty good bet that these builds do not
    # have 64-bit pointers.
    _int_types += (c_int64,)
for t in _int_types:
    if sizeof(t) == sizeof(c_size_t):
        c_ptrdiff_t = t
del t
del _int_types

class c_void(Structure):
    # c_void_p is a buggy return type, converting to int, so
    # POINTER(None) == c_void_p is actually written as
    # POINTER(c_void), so it can be treated as a real pointer.
    _fields_ = [('dummy', c_int)]

def POINTER(obj):
    p = ctypes.POINTER(obj)

    # Convert None to a real NULL pointer to work around bugs
    # in how ctypes handles None on 64-bit platforms
    if not isinstance(p.from_param, classmethod):
        def from_param(cls, x):
            if x is None:
                return cls()
            else:
                return x
        p.from_param = classmethod(from_param)

    return p

class UserString:
    def __init__(self, seq):
        if isinstance(seq, basestring):
            self.data = seq
        elif isinstance(seq, UserString):
            self.data = seq.data[:]
        else:
            self.data = str(seq)
    def __str__(self): return str(self.data)
    def __repr__(self): return repr(self.data)
    def __int__(self): return int(self.data)
    def __long__(self): return long(self.data)
    def __float__(self): return float(self.data)
    def __complex__(self): return complex(self.data)
    def __hash__(self): return hash(self.data)

    def __cmp__(self, string):
        if isinstance(string, UserString):
            return cmp(self.data, string.data)
        else:
            return cmp(self.data, string)
    def __contains__(self, char):
        return char in self.data

    def __len__(self): return len(self.data)
    def __getitem__(self, index): return self.__class__(self.data[index])
    def __getslice__(self, start, end):
        start = max(start, 0); end = max(end, 0)
        return self.__class__(self.data[start:end])

    def __add__(self, other):
        if isinstance(other, UserString):
            return self.__class__(self.data + other.data)
        elif isinstance(other, basestring):
            return self.__class__(self.data + other)
        else:
            return self.__class__(self.data + str(other))
    def __radd__(self, other):
        if isinstance(other, basestring):
            return self.__class__(other + self.data)
        else:
            return self.__class__(str(other) + self.data)
    def __mul__(self, n):
        return self.__class__(self.data*n)
    __rmul__ = __mul__
    def __mod__(self, args):
        return self.__class__(self.data % args)

    # the following methods are defined in alphabetical order:
    def capitalize(self): return self.__class__(self.data.capitalize())
    def center(self, width, *args):
        return self.__class__(self.data.center(width, *args))
    def count(self, sub, start=0, end=sys.maxsize):
        return self.data.count(sub, start, end)
    def decode(self, encoding=None, errors=None): # XXX improve this?
        if encoding:
            if errors:
                return self.__class__(self.data.decode(encoding, errors))
            else:
                return self.__class__(self.data.decode(encoding))
        else:
            return self.__class__(self.data.decode())
    def encode(self, encoding=None, errors=None): # XXX improve this?
        if encoding:
            if errors:
                return self.__class__(self.data.encode(encoding, errors))
            else:
                return self.__class__(self.data.encode(encoding))
        else:
            return self.__class__(self.data.encode())
    def endswith(self, suffix, start=0, end=sys.maxsize):
        return self.data.endswith(suffix, start, end)
    def expandtabs(self, tabsize=8):
        return self.__class__(self.data.expandtabs(tabsize))
    def find(self, sub, start=0, end=sys.maxsize):
        return self.data.find(sub, start, end)
    def index(self, sub, start=0, end=sys.maxsize):
        return self.data.index(sub, start, end)
    def isalpha(self): return self.data.isalpha()
    def isalnum(self): return self.data.isalnum()
    def isdecimal(self): return self.data.isdecimal()
    def isdigit(self): return self.data.isdigit()
    def islower(self): return self.data.islower()
    def isnumeric(self): return self.data.isnumeric()
    def isspace(self): return self.data.isspace()
    def istitle(self): return self.data.istitle()
    def isupper(self): return self.data.isupper()
    def join(self, seq): return self.data.join(seq)
    def ljust(self, width, *args):
        return self.__class__(self.data.ljust(width, *args))
    def lower(self): return self.__class__(self.data.lower())
    def lstrip(self, chars=None): return self.__class__(self.data.lstrip(chars))
    def partition(self, sep):
        return self.data.partition(sep)
    def replace(self, old, new, maxsplit=-1):
        return self.__class__(self.data.replace(old, new, maxsplit))
    def rfind(self, sub, start=0, end=sys.maxsize):
        return self.data.rfind(sub, start, end)
    def rindex(self, sub, start=0, end=sys.maxsize):
        return self.data.rindex(sub, start, end)
    def rjust(self, width, *args):
        return self.__class__(self.data.rjust(width, *args))
    def rpartition(self, sep):
        return self.data.rpartition(sep)
    def rstrip(self, chars=None): return self.__class__(self.data.rstrip(chars))
    def split(self, sep=None, maxsplit=-1):
        return self.data.split(sep, maxsplit)
    def rsplit(self, sep=None, maxsplit=-1):
        return self.data.rsplit(sep, maxsplit)
    def splitlines(self, keepends=0): return self.data.splitlines(keepends)
    def startswith(self, prefix, start=0, end=sys.maxsize):
        return self.data.startswith(prefix, start, end)
    def strip(self, chars=None): return self.__class__(self.data.strip(chars))
    def swapcase(self): return self.__class__(self.data.swapcase())
    def title(self): return self.__class__(self.data.title())
    def translate(self, *args):
        return self.__class__(self.data.translate(*args))
    def upper(self): return self.__class__(self.data.upper())
    def zfill(self, width): return self.__class__(self.data.zfill(width))

class MutableString(UserString):
    """mutable string objects

    Python strings are immutable objects.  This has the advantage, that
    strings may be used as dictionary keys.  If this property isn't needed
    and you insist on changing string values in place instead, you may cheat
    and use MutableString.

    But the purpose of this class is an educational one: to prevent
    people from inventing their own mutable string class derived
    from UserString and than forget thereby to remove (override) the
    __hash__ method inherited from UserString.  This would lead to
    errors that would be very hard to track down.

    A faster and better solution is to rewrite your program using lists."""
    def __init__(self, string=""):
        self.data = string
    def __hash__(self):
        raise TypeError("unhashable type (it is mutable)")
    def __setitem__(self, index, sub):
        if index < 0:
            index += len(self.data)
        if index < 0 or index >= len(self.data): raise IndexError
        self.data = self.data[:index] + sub + self.data[index+1:]
    def __delitem__(self, index):
        if index < 0:
            index += len(self.data)
        if index < 0 or index >= len(self.data): raise IndexError
        self.data = self.data[:index] + self.data[index+1:]
    def __setslice__(self, start, end, sub):
        start = max(start, 0); end = max(end, 0)
        if isinstance(sub, UserString):
            self.data = self.data[:start]+sub.data+self.data[end:]
        elif isinstance(sub, basestring):
            self.data = self.data[:start]+sub+self.data[end:]
        else:
            self.data =  self.data[:start]+str(sub)+self.data[end:]
    def __delslice__(self, start, end):
        start = max(start, 0); end = max(end, 0)
        self.data = self.data[:start] + self.data[end:]
    def immutable(self):
        return UserString(self.data)
    def __iadd__(self, other):
        if isinstance(other, UserString):
            self.data += other.data
        elif isinstance(other, basestring):
            self.data += other
        else:
            self.data += str(other)
        return self
    def __imul__(self, n):
        self.data *= n
        return self

class String(MutableString, Union):

    _fields_ = [('raw', POINTER(c_char)),
                ('data', c_char_p)]

    def __init__(self, obj=""):
        if isinstance(obj, (str, UserString)):
            self.data = obj.encode('utf-8')
        else:
            self.raw = obj

    def __len__(self):
        return self.data and len(self.data) or 0

    def from_param(cls, obj):
        # Convert None or 0
        if obj is None or obj == 0:
            return cls(POINTER(c_char)())

        # Convert from String
        elif isinstance(obj, String):
            return obj

        # Convert from str
        elif isinstance(obj, str):
            return cls(obj)

        # Convert from c_char_p
        elif isinstance(obj, c_char_p):
            return obj

        # Convert from POINTER(c_char)
        elif isinstance(obj, POINTER(c_char)):
            return obj

        # Convert from raw pointer
        elif isinstance(obj, int):
            return cls(cast(obj, POINTER(c_char)))

        # Convert from object
        else:
            return String.from_param(obj._as_parameter_)
    from_param = classmethod(from_param)

def ReturnString(obj, func=None, arguments=None):
    return String.from_param(obj)

# As of ctypes 1.0, ctypes does not support custom error-checking
# functions on callbacks, nor does it support custom datatypes on
# callbacks, so we must ensure that all callbacks return
# primitive datatypes.
#
# Non-primitive return values wrapped with UNCHECKED won't be
# typechecked, and will be converted to c_void_p.
def UNCHECKED(type):
    if (hasattr(type, "_type_") and isinstance(type._type_, str)
        and type._type_ != "P"):
        return type
    else:
        return c_void_p

# ctypes doesn't have direct support for variadic functions, so we have to write
# our own wrapper class
class _variadic_function(object):
    def __init__(self,func,restype,argtypes):
        self.func=func
        self.func.restype=restype
        self.argtypes=argtypes
    def _as_parameter_(self):
        # So we can pass this variadic function as a function pointer
        return self.func
    def __call__(self,*args):
        fixed_args=[]
        i=0
        for argtype in self.argtypes:
            # Typecheck what we can
            fixed_args.append(argtype.from_param(args[i]))
            i+=1
        return self.func(*fixed_args+list(args[i:]))

# End preamble

_libs = {}

# Begin loader

# ----------------------------------------------------------------------------
# Copyright (c) 2008 David James
# Copyright (c) 2006-2008 Alex Holkner
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#  * Neither the name of pyglet nor the names of its
#    contributors may be used to endorse or promote products
#    derived from this software without specific prior written
#    permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
# ----------------------------------------------------------------------------

import os.path, re, sys, glob
import platform
import ctypes
import ctypes.util

def _environ_path(name):
    if name in os.environ:
        return os.environ[name].split(":")
    else:
        return []

class LibraryLoader(object):
    def __init__(self):
        self.other_dirs=[]

    def load_library(self,libname):
        """Given the name of a library, load it."""
        paths = self.getpaths(libname)

        for path in paths:
            if os.path.exists(path):
                return self.load(path)

        raise ImportError("%s not found." % libname)

    def load(self,path):
        """Given a path to a library, load it."""
        try:
            # Darwin requires dlopen to be called with mode RTLD_GLOBAL instead
            # of the default RTLD_LOCAL.  Without this, you end up with
            # libraries not being loadable, resulting in "Symbol not found"
            # errors
            if sys.platform == 'darwin':
                return ctypes.CDLL(path, ctypes.RTLD_GLOBAL)
            else:
                return ctypes.cdll.LoadLibrary(path)
        except OSError as e:
            raise ImportError(e)

    def getpaths(self,libname):
        """Return a list of paths where the library might be found."""
        if os.path.isabs(libname):
            yield libname
        else:
            # FIXME / TODO return '.' and os.path.dirname(__file__)
            for path in self.getplatformpaths(libname):
                yield path

            path = ctypes.util.find_library(libname)
            if path: yield path

    def getplatformpaths(self, libname):
        return []

# Darwin (Mac OS X)

class DarwinLibraryLoader(LibraryLoader):
    name_formats = ["lib%s.dylib", "lib%s.so", "lib%s.bundle", "%s.dylib",
                "%s.so", "%s.bundle", "%s"]

    def getplatformpaths(self,libname):
        if os.path.pathsep in libname:
            names = [libname]
        else:
            names = [format % libname for format in self.name_formats]

        for dir in self.getdirs(libname):
            for name in names:
                yield os.path.join(dir,name)

    def getdirs(self,libname):
        '''Implements the dylib search as specified in Apple documentation:

        http://developer.apple.com/documentation/DeveloperTools/Conceptual/
            DynamicLibraries/Articles/DynamicLibraryUsageGuidelines.html

        Before commencing the standard search, the method first checks
        the bundle's ``Frameworks`` directory if the application is running
        within a bundle (OS X .app).
        '''

        dyld_fallback_library_path = _environ_path("DYLD_FALLBACK_LIBRARY_PATH")
        if not dyld_fallback_library_path:
            dyld_fallback_library_path = [os.path.expanduser('~/lib'),
                                          '/usr/local/lib', '/usr/lib']

        dirs = []

        if '/' in libname:
            dirs.extend(_environ_path("DYLD_LIBRARY_PATH"))
        else:
            dirs.extend(_environ_path("LD_LIBRARY_PATH"))
            dirs.extend(_environ_path("DYLD_LIBRARY_PATH"))

        dirs.extend(self.other_dirs)
        dirs.append(".")
        dirs.append(os.path.dirname(__file__))

        if hasattr(sys, 'frozen') and sys.frozen == 'macosx_app':
            dirs.append(os.path.join(
                os.environ['RESOURCEPATH'],
                '..',
                'Frameworks'))

        dirs.extend(dyld_fallback_library_path)

        return dirs

# Posix

class PosixLibraryLoader(LibraryLoader):
    _ld_so_cache = None

    def _create_ld_so_cache(self):
        # Recreate search path followed by ld.so.  This is going to be
        # slow to build, and incorrect (ld.so uses ld.so.cache, which may
        # not be up-to-date).  Used only as fallback for distros without
        # /sbin/ldconfig.
        #
        # We assume the DT_RPATH and DT_RUNPATH binary sections are omitted.

        directories = []
        for name in ("LD_LIBRARY_PATH",
                     "SHLIB_PATH", # HPUX
                     "LIBPATH", # OS/2, AIX
                     "LIBRARY_PATH", # BE/OS
                    ):
            if name in os.environ:
                directories.extend(os.environ[name].split(os.pathsep))
        directories.extend(self.other_dirs)
        directories.append(".")
        directories.append(os.path.dirname(__file__))

        try: directories.extend([dir.strip() for dir in open('/etc/ld.so.conf')])
        except IOError: pass

        unix_lib_dirs_list = ['/lib', '/usr/lib', '/lib64', '/usr/lib64']
        if sys.platform.startswith('linux'):
            # Try and support multiarch work in Ubuntu
            # https://wiki.ubuntu.com/MultiarchSpec
            bitage = platform.architecture()[0]
            if bitage.startswith('32'):
                # Assume Intel/AMD x86 compat
                unix_lib_dirs_list += ['/lib/i386-linux-gnu', '/usr/lib/i386-linux-gnu']
            elif bitage.startswith('64'):
                # Assume Intel/AMD x86 compat
                unix_lib_dirs_list += ['/lib/x86_64-linux-gnu', '/usr/lib/x86_64-linux-gnu']
            else:
                # guess...
                unix_lib_dirs_list += glob.glob('/lib/*linux-gnu')
        directories.extend(unix_lib_dirs_list)

        cache = {}
        lib_re = re.compile(r'lib(.*)\.s[ol]')
        ext_re = re.compile(r'\.s[ol]$')
        for dir in directories:
            try:
                for path in glob.glob("%s/*.s[ol]*" % dir):
                    file = os.path.basename(path)

                    # Index by filename
                    if file not in cache:
                        cache[file] = path

                    # Index by library name
                    match = lib_re.match(file)
                    if match:
                        library = match.group(1)
                        if library not in cache:
                            cache[library] = path
            except OSError:
                pass

        self._ld_so_cache = cache

    def getplatformpaths(self, libname):
        if self._ld_so_cache is None:
            self._create_ld_so_cache()

        result = self._ld_so_cache.get(libname)
        if result: yield result

        path = ctypes.util.find_library(libname)
        if path: yield os.path.join("/lib",path)

# Windows

class _WindowsLibrary(object):
    def __init__(self, path):
        self.cdll = ctypes.cdll.LoadLibrary(path)
        self.windll = ctypes.windll.LoadLibrary(path)

    def __getattr__(self, name):
        try: return getattr(self.cdll,name)
        except AttributeError:
            try: return getattr(self.windll,name)
            except AttributeError:
                raise

class WindowsLibraryLoader(LibraryLoader):
    name_formats = ["%s.dll", "lib%s.dll", "%slib.dll"]

    def load_library(self, libname):
        try:
            result = LibraryLoader.load_library(self, libname)
        except ImportError:
            result = None
            if os.path.sep not in libname:
                for name in self.name_formats:
                    try:
                        result = getattr(ctypes.cdll, name % libname)
                        if result:
                            break
                    except WindowsError:
                        result = None
            if result is None:
                try:
                    result = getattr(ctypes.cdll, libname)
                except WindowsError:
                    result = None
            if result is None:
                raise ImportError("%s not found." % libname)
        return result

    def load(self, path):
        return _WindowsLibrary(path)

    def getplatformpaths(self, libname):
        if os.path.sep not in libname:
            for name in self.name_formats:
                dll_in_current_dir = os.path.abspath(name % libname)
                if os.path.exists(dll_in_current_dir):
                    yield dll_in_current_dir
                path = ctypes.util.find_library(name % libname)
                if path:
                    yield path

# Platform switching

# If your value of sys.platform does not appear in this dict, please contact
# the Ctypesgen maintainers.

loaderclass = {
    "darwin":   DarwinLibraryLoader,
    "cygwin":   WindowsLibraryLoader,
    "win32":    WindowsLibraryLoader
}

loader = loaderclass.get(sys.platform, PosixLibraryLoader)()

def add_library_search_dirs(other_dirs):
    loader.other_dirs = other_dirs

load_library = loader.load_library

del loaderclass

# End loader

bindir = os.path.dirname(__file__) + os.path.sep + '..' + os.path.sep + '..' + os.path.sep + 'bin'
add_library_search_dirs([bindir])

# Begin libraries

_libs["SPH"] = load_library("SPH")

# 1 libraries
# End libraries

# No modules

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/3rdparty/whereami.h: 38
if hasattr(_libs['SPH'], 'wai_getExecutablePath'):
    wai_getExecutablePath = _libs['SPH'].wai_getExecutablePath
    wai_getExecutablePath.argtypes = [String, c_int, POINTER(c_int)]
    wai_getExecutablePath.restype = c_int

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/3rdparty/whereami.h: 59
if hasattr(_libs['SPH'], 'wai_getModulePath'):
    wai_getModulePath = _libs['SPH'].wai_getModulePath
    wai_getModulePath.argtypes = [String, c_int, POINTER(c_int)]
    wai_getModulePath.restype = c_int

enum_anon_1 = c_int # /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/datatypes.h: 13

T_CHAR = 0 # /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/datatypes.h: 13

T_UINT = (T_CHAR + 1) # /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/datatypes.h: 13

T_INT = (T_UINT + 1) # /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/datatypes.h: 13

T_FLOAT32 = (T_INT + 1) # /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/datatypes.h: 13

T_FLOAT64 = (T_FLOAT32 + 1) # /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/datatypes.h: 13

TYPE_COUNT = (T_FLOAT64 + 1) # /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/datatypes.h: 13

FieldType = enum_anon_1 # /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/datatypes.h: 13

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/datatypes.h: 17
class union_anon_2(Union):
    pass

union_anon_2.__slots__ = [
    'c',
    'u',
    'i',
    'f4',
    'f8',
    'ptr',
]
union_anon_2._fields_ = [
    ('c', String),
    ('u', POINTER(c_uint32)),
    ('i', POINTER(c_int32)),
    ('f4', POINTER(c_float)),
    ('f8', POINTER(c_double)),
    ('ptr', POINTER(None)),
]

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/datatypes.h: 25
class struct_anon_3(Structure):
    pass

struct_anon_3.__slots__ = [
    'type',
    'unnamed_1',
]
struct_anon_3._anonymous_ = [
    'unnamed_1',
]
struct_anon_3._fields_ = [
    ('type', FieldType),
    ('unnamed_1', union_anon_2),
]

DataPtr = struct_anon_3 # /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/datatypes.h: 25

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/particle_system.h: 48
class struct_anon_4(Structure):
    pass

struct_anon_4.__slots__ = [
    'num_fields',
    'names',
    'names_offsets',
    'dimensions',
    'num_dimensions',
    'dimensions_offsets',
    'entry_sizes',
    'data',
    'data_sizes',
    'data_offsets',
    'data_ptrs',
    'num_host_fields',
    'host_names',
    'host_data',
    'host_data_size',
]
struct_anon_4._fields_ = [
    ('num_fields', c_uint),
    ('names', String),
    ('names_offsets', POINTER(c_uint)),
    ('dimensions', POINTER(c_uint)),
    ('num_dimensions', POINTER(c_uint)),
    ('dimensions_offsets', POINTER(c_uint)),
    ('entry_sizes', POINTER(c_uint)),
    ('data', POINTER(None)),
    ('data_sizes', POINTER(c_uint)),
    ('data_offsets', POINTER(c_uint)),
    ('data_ptrs', POINTER(DataPtr)),
    ('num_host_fields', c_uint),
    ('host_names', POINTER(POINTER(c_char))),
    ('host_data', POINTER(POINTER(None))),
    ('host_data_size', POINTER(c_uint)),
]

psdata = struct_anon_4 # /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/particle_system.h: 48

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/particle_system.h: 56
if hasattr(_libs['SPH'], 'display_psdata'):
    display_psdata = _libs['SPH'].display_psdata
    display_psdata.argtypes = [psdata, POINTER(POINTER(c_char))]
    display_psdata.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/particle_system.h: 58
for _lib in iter(_libs.values()):
    if not hasattr(_lib, 'init_psdata_fluid'):
        continue
    init_psdata_fluid = _lib.init_psdata_fluid
    init_psdata_fluid.argtypes = [POINTER(psdata), c_int, c_double, c_double, c_double, c_double, c_double, c_double, c_double, c_double, c_double]
    init_psdata_fluid.restype = None
    break

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/particle_system.h: 61
if hasattr(_libs['SPH'], 'get_field_psdata'):
    get_field_psdata = _libs['SPH'].get_field_psdata
    get_field_psdata.argtypes = [psdata, String]
    get_field_psdata.restype = c_int

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/particle_system.h: 62
if hasattr(_libs['SPH'], 'set_field_psdata'):
    set_field_psdata = _libs['SPH'].set_field_psdata
    set_field_psdata.argtypes = [POINTER(psdata), String, POINTER(None), c_uint, c_uint]
    set_field_psdata.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/particle_system.h: 64
if hasattr(_libs['SPH'], 'psdata_names_size'):
    psdata_names_size = _libs['SPH'].psdata_names_size
    psdata_names_size.argtypes = [psdata]
    psdata_names_size.restype = c_uint

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/particle_system.h: 65
if hasattr(_libs['SPH'], 'psdata_dimensions_size'):
    psdata_dimensions_size = _libs['SPH'].psdata_dimensions_size
    psdata_dimensions_size.argtypes = [psdata]
    psdata_dimensions_size.restype = c_uint

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/particle_system.h: 66
if hasattr(_libs['SPH'], 'psdata_data_size'):
    psdata_data_size = _libs['SPH'].psdata_data_size
    psdata_data_size.argtypes = [psdata]
    psdata_data_size.restype = c_uint

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/particle_system.h: 68
if hasattr(_libs['SPH'], 'create_host_field_psdata'):
    create_host_field_psdata = _libs['SPH'].create_host_field_psdata
    create_host_field_psdata.argtypes = [POINTER(psdata), String, POINTER(None), c_uint]
    create_host_field_psdata.restype = c_int

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/particle_system.h: 69
if hasattr(_libs['SPH'], 'get_host_field_psdata'):
    get_host_field_psdata = _libs['SPH'].get_host_field_psdata
    get_host_field_psdata.argtypes = [POINTER(psdata), String]
    get_host_field_psdata.restype = c_int

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/particle_system.h: 77
if hasattr(_libs['SPH'], 'free_psdata'):
    free_psdata = _libs['SPH'].free_psdata
    free_psdata.argtypes = [POINTER(psdata)]
    free_psdata.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/build_psdata.h: 6
if hasattr(_libs['SPH'], 'build_psdata_from_string'):
    build_psdata_from_string = _libs['SPH'].build_psdata_from_string
    build_psdata_from_string.argtypes = [POINTER(psdata), String]
    build_psdata_from_string.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/build_psdata.h: 7
if hasattr(_libs['SPH'], 'build_psdata'):
    build_psdata = _libs['SPH'].build_psdata
    build_psdata.argtypes = [POINTER(psdata), String]
    build_psdata.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/config.h: 4
if hasattr(_libs['SPH'], 'load_config'):
    load_config = _libs['SPH'].load_config
    load_config.argtypes = [String]
    load_config.restype = c_int

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/config.h: 5
if hasattr(_libs['SPH'], 'get_config_section'):
    get_config_section = _libs['SPH'].get_config_section
    get_config_section.argtypes = [String]
    if sizeof(c_int) == sizeof(c_void_p):
        get_config_section.restype = ReturnString
    else:
        get_config_section.restype = String
        get_config_section.errcheck = ReturnString

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/config.h: 6
if hasattr(_libs['SPH'], 'unload_config'):
    unload_config = _libs['SPH'].unload_config
    unload_config.argtypes = []
    unload_config.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/note.h: 4
if hasattr(_libs['SPH'], 'set_log_level'):
    set_log_level = _libs['SPH'].set_log_level
    set_log_level.argtypes = [c_uint]
    set_log_level.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/note.h: 5
if hasattr(_libs['SPH'], 'note'):
    _func = _libs['SPH'].note
    _restype = None
    _argtypes = [c_uint, String]
    note = _variadic_function(_func,_restype,_argtypes)

cl_int = c_int32 # /usr/include/CL/cl_platform.h: 268

cl_uint = c_uint32 # /usr/include/CL/cl_platform.h: 269

cl_ulong = c_uint64 # /usr/include/CL/cl_platform.h: 271

# /usr/include/CL/cl.h: 39
class struct__cl_platform_id(Structure):
    pass

cl_platform_id = POINTER(struct__cl_platform_id) # /usr/include/CL/cl.h: 39

# /usr/include/CL/cl.h: 40
class struct__cl_device_id(Structure):
    pass

cl_device_id = POINTER(struct__cl_device_id) # /usr/include/CL/cl.h: 40

# /usr/include/CL/cl.h: 43
class struct__cl_mem(Structure):
    pass

cl_mem = POINTER(struct__cl_mem) # /usr/include/CL/cl.h: 43

# /usr/include/CL/cl.h: 44
class struct__cl_program(Structure):
    pass

cl_program = POINTER(struct__cl_program) # /usr/include/CL/cl.h: 44

# /usr/include/CL/cl.h: 45
class struct__cl_kernel(Structure):
    pass

cl_kernel = POINTER(struct__cl_kernel) # /usr/include/CL/cl.h: 45

cl_bool = cl_uint # /usr/include/CL/cl.h: 49

cl_bitfield = cl_ulong # /usr/include/CL/cl.h: 50

cl_device_type = cl_bitfield # /usr/include/CL/cl.h: 51

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/clerror.h: 10
if hasattr(_libs['SPH'], 'printCLError'):
    printCLError = _libs['SPH'].printCLError
    printCLError.argtypes = [cl_int]
    printCLError.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/clerror.h: 11
if hasattr(_libs['SPH'], 'contextErrorCallback'):
    contextErrorCallback = _libs['SPH'].contextErrorCallback
    contextErrorCallback.argtypes = [String, POINTER(None), c_size_t, POINTER(None)]
    contextErrorCallback.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/particle_system_host.h: 47
class struct_anon_63(Structure):
    pass

struct_anon_63.__slots__ = [
    'host_psdata',
    'num_fields',
    'names',
    'names_offsets',
    'dimensions',
    'num_dimensions',
    'dimensions_offsets',
    'field_types',
    'entry_sizes',
    'data',
    'data_sizes',
    'data_offsets',
    'block_totals',
    'backup_prefix_sum',
    'num_kernels',
    'kernel_names',
    'kernels',
    'ps_prog',
    'num_grid_cells',
    'po2_workgroup_size',
    'num_blocks',
]
struct_anon_63._fields_ = [
    ('host_psdata', psdata),
    ('num_fields', c_uint),
    ('names', cl_mem),
    ('names_offsets', cl_mem),
    ('dimensions', cl_mem),
    ('num_dimensions', cl_mem),
    ('dimensions_offsets', cl_mem),
    ('field_types', cl_mem),
    ('entry_sizes', cl_mem),
    ('data', cl_mem),
    ('data_sizes', cl_mem),
    ('data_offsets', cl_mem),
    ('block_totals', cl_mem),
    ('backup_prefix_sum', cl_mem),
    ('num_kernels', c_size_t),
    ('kernel_names', POINTER(POINTER(c_char))),
    ('kernels', POINTER(cl_kernel)),
    ('ps_prog', cl_program),
    ('num_grid_cells', c_uint),
    ('po2_workgroup_size', c_size_t),
    ('num_blocks', c_uint),
]

psdata_opencl = struct_anon_63 # /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/particle_system_host.h: 47

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/particle_system_host.h: 55
if hasattr(_libs['SPH'], 'init_opencl'):
    init_opencl = _libs['SPH'].init_opencl
    init_opencl.argtypes = []
    init_opencl.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/particle_system_host.h: 56
if hasattr(_libs['SPH'], 'create_psdata_opencl'):
    create_psdata_opencl = _libs['SPH'].create_psdata_opencl
    create_psdata_opencl.argtypes = [POINTER(psdata), String]
    create_psdata_opencl.restype = psdata_opencl

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/particle_system_host.h: 57
if hasattr(_libs['SPH'], 'assign_pso_kernel_args'):
    assign_pso_kernel_args = _libs['SPH'].assign_pso_kernel_args
    assign_pso_kernel_args.argtypes = [psdata_opencl]
    assign_pso_kernel_args.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/particle_system_host.h: 58
if hasattr(_libs['SPH'], 'set_kernel_args_to_pso'):
    set_kernel_args_to_pso = _libs['SPH'].set_kernel_args_to_pso
    set_kernel_args_to_pso.argtypes = [psdata_opencl, cl_kernel]
    set_kernel_args_to_pso.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/particle_system_host.h: 59
if hasattr(_libs['SPH'], 'free_psdata_opencl'):
    free_psdata_opencl = _libs['SPH'].free_psdata_opencl
    free_psdata_opencl.argtypes = [POINTER(psdata_opencl)]
    free_psdata_opencl.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/particle_system_host.h: 60
if hasattr(_libs['SPH'], 'terminate_opencl'):
    terminate_opencl = _libs['SPH'].terminate_opencl
    terminate_opencl.argtypes = []
    terminate_opencl.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/particle_system_host.h: 62
if hasattr(_libs['SPH'], 'sync_psdata_device_to_host'):
    sync_psdata_device_to_host = _libs['SPH'].sync_psdata_device_to_host
    sync_psdata_device_to_host.argtypes = [psdata, psdata_opencl]
    sync_psdata_device_to_host.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/particle_system_host.h: 63
if hasattr(_libs['SPH'], 'sync_psdata_fields_device_to_host'):
    sync_psdata_fields_device_to_host = _libs['SPH'].sync_psdata_fields_device_to_host
    sync_psdata_fields_device_to_host.argtypes = [psdata, psdata_opencl, c_size_t, POINTER(POINTER(c_char))]
    sync_psdata_fields_device_to_host.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/particle_system_host.h: 64
if hasattr(_libs['SPH'], 'sync_psdata_host_to_device'):
    sync_psdata_host_to_device = _libs['SPH'].sync_psdata_host_to_device
    sync_psdata_host_to_device.argtypes = [psdata, psdata_opencl, c_int]
    sync_psdata_host_to_device.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/particle_system_host.h: 65
if hasattr(_libs['SPH'], 'sync_psdata_fields_host_to_device'):
    sync_psdata_fields_host_to_device = _libs['SPH'].sync_psdata_fields_host_to_device
    sync_psdata_fields_host_to_device.argtypes = [psdata, psdata_opencl, c_size_t, POINTER(POINTER(c_char))]
    sync_psdata_fields_host_to_device.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/particle_system_host.h: 67
if hasattr(_libs['SPH'], 'populate_position_cuboid_device_opencl'):
    populate_position_cuboid_device_opencl = _libs['SPH'].populate_position_cuboid_device_opencl
    populate_position_cuboid_device_opencl.argtypes = [psdata_opencl, c_float, c_float, c_float, c_float, c_float, c_float, c_uint, c_uint, c_uint]
    populate_position_cuboid_device_opencl.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/particle_system_host.h: 73
if hasattr(_libs['SPH'], 'rotate_particles_device_opencl'):
    rotate_particles_device_opencl = _libs['SPH'].rotate_particles_device_opencl
    rotate_particles_device_opencl.argtypes = [psdata_opencl, c_float, c_float, c_float]
    rotate_particles_device_opencl.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/particle_system_host.h: 74
if hasattr(_libs['SPH'], 'call_for_all_particles_device_opencl'):
    call_for_all_particles_device_opencl = _libs['SPH'].call_for_all_particles_device_opencl
    call_for_all_particles_device_opencl.argtypes = [psdata_opencl, String]
    call_for_all_particles_device_opencl.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/particle_system_host.h: 76
if hasattr(_libs['SPH'], 'compute_particle_bins_device_opencl'):
    compute_particle_bins_device_opencl = _libs['SPH'].compute_particle_bins_device_opencl
    compute_particle_bins_device_opencl.argtypes = [psdata_opencl]
    compute_particle_bins_device_opencl.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/particle_system_host.h: 78
if hasattr(_libs['SPH'], 'compute_density_device_opencl'):
    compute_density_device_opencl = _libs['SPH'].compute_density_device_opencl
    compute_density_device_opencl.argtypes = [psdata_opencl]
    compute_density_device_opencl.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/particle_system_host.h: 79
if hasattr(_libs['SPH'], 'compute_forces_device_opencl'):
    compute_forces_device_opencl = _libs['SPH'].compute_forces_device_opencl
    compute_forces_device_opencl.argtypes = [psdata_opencl]
    compute_forces_device_opencl.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/particle_system_host.h: 80
if hasattr(_libs['SPH'], 'step_forward_device_opencl'):
    step_forward_device_opencl = _libs['SPH'].step_forward_device_opencl
    step_forward_device_opencl.argtypes = [psdata_opencl]
    step_forward_device_opencl.restype = None

if hasattr(_libs['SPH'], 'set_body_position_device_opencl'):
    set_body_position_device_opencl = _libs['SPH'].set_body_position_device_opencl
    set_body_position_device_opencl.argtypes = [psdata_opencl, c_uint, ndpointer(c_float, flags="C_CONTIGUOUS")]
    set_body_position_device_opencl.restype = None

if hasattr(_libs['SPH'], 'set_body_rotation_device_opencl'):
    set_body_rotation_device_opencl = _libs['SPH'].set_body_rotation_device_opencl
    set_body_rotation_device_opencl.argtypes = [psdata_opencl, c_uint, ndpointer(c_float, flags="C_CONTIGUOUS")]
    set_body_rotation_device_opencl.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/platforminfo.h: 22
class struct_anon_64(Structure):
    pass

struct_anon_64.__slots__ = [
    'id',
    'name',
    'type',
    'opencl_version',
    'driver_version',
    'extensions',
    'compiler_available',
    'linker_available',
    'profile',
    'max_compute_units',
    'max_workgroup_size',
    'max_work_item_dimensions',
    'max_work_item_sizes',
    'pref_vector_width',
    'double_supported',
]
struct_anon_64._fields_ = [
    ('id', cl_device_id),
    ('name', String),
    ('type', cl_device_type),
    ('opencl_version', String),
    ('driver_version', String),
    ('extensions', String),
    ('compiler_available', cl_bool),
    ('linker_available', cl_bool),
    ('profile', String),
    ('max_compute_units', cl_uint),
    ('max_workgroup_size', cl_uint),
    ('max_work_item_dimensions', cl_uint),
    ('max_work_item_sizes', POINTER(c_size_t)),
    ('pref_vector_width', cl_uint),
    ('double_supported', cl_bool),
]

Device = struct_anon_64 # /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/platforminfo.h: 22

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/platforminfo.h: 29
class struct_anon_65(Structure):
    pass

struct_anon_65.__slots__ = [
    'id',
    'name',
    'devices',
    'num_devices',
]
struct_anon_65._fields_ = [
    ('id', cl_platform_id),
    ('name', String),
    ('devices', POINTER(Device)),
    ('num_devices', c_uint),
]

Platform = struct_anon_65 # /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/platforminfo.h: 29

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/platforminfo.h: 31
if hasattr(_libs['SPH'], 'get_opencl_platform_info'):
    get_opencl_platform_info = _libs['SPH'].get_opencl_platform_info
    get_opencl_platform_info.argtypes = [POINTER(POINTER(Platform)), POINTER(c_uint)]
    get_opencl_platform_info.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/opencl/platforminfo.h: 32
if hasattr(_libs['SPH'], 'free_opencl_platform_info'):
    free_opencl_platform_info = _libs['SPH'].free_opencl_platform_info
    free_opencl_platform_info.argtypes = []
    free_opencl_platform_info.restype = None

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/stringly.h: 8
if hasattr(_libs['SPH'], 'allocate_string_typed_array'):
    allocate_string_typed_array = _libs['SPH'].allocate_string_typed_array
    allocate_string_typed_array.argtypes = [String, c_size_t]
    allocate_string_typed_array.restype = POINTER(None)

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/stringly.h: 9
if hasattr(_libs['SPH'], 'typeof_string_type'):
    typeof_string_type = _libs['SPH'].typeof_string_type
    typeof_string_type.argtypes = [String]
    typeof_string_type.restype = FieldType

# /media/HDD/Adam/Documents/Programming/Project/opencl-sph-private/src/stringly.h: 10
if hasattr(_libs['SPH'], 'sizeof_string_type'):
    sizeof_string_type = _libs['SPH'].sizeof_string_type
    sizeof_string_type.argtypes = [String]
    sizeof_string_type.restype = c_size_t

# No inserted files
