// Functions from C wasm code
export var find_minimums = Module.cwrap('find_minimums', 'number', ["string"]);
export var get_minimum = Module.cwrap('get_minimum', "string", ["number"]);
export var number_minimums = Module.cwrap('number_minimums', "number");
export var minimum_score = Module.cwrap('minimum_score', 'number');
export var set_weights = Module.cwrap('set_weights', null, ["number", "number", "number", "number", "number"]);
