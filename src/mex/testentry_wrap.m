function a = testentry_wrap()

entrypath = which('testentry');
entrypath = fileparts(entrypath);
setenv('EXE_PATH', entrypath);

a = testentry();