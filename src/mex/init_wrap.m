function init_wrap(path)

entrypath = which('init');
entrypath = fileparts(entrypath);
setenv('EXE_PATH', entrypath);

init(path);