function init_wrap(path)

entrypath = which('initsph');
entrypath = fileparts(entrypath);
setenv('EXE_PATH', entrypath);

initsph(path);