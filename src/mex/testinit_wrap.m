function testinit_wrap()

entrypath = which('testinit');
entrypath = fileparts(entrypath);
setenv('EXE_PATH', entrypath);

testinit();
