//specify path for server
Module['locateFile'] = function(path, prefix) {
  // TODO: find better option than hardcoded
  return "/assets/compute/" + prefix + path;
}
