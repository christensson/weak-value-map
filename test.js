'use strict';

var WeakValueMap = require('./');
var test = require('tape');

test('map', function(assert) {
	var map = new WeakValueMap();
	map.set(1, 'one')
	   .set(2, 2)
	   .set(3, true)
	   .set(4, false);
	var d1 = new Date();
	var d2 = new Date(d1);
	map.set(5, d1);
	map.set(6, {6: 'six'});
	assert.equal(map.get(1), 'one');
	assert.strictEqual(map.get(2), 2);
	assert.strictEqual(map.get(3), true);
	assert.strictEqual(map.get(4), false);
	assert.strictEqual(map.get(5).getTime(), d2.getTime());
	assert.deepEqual(map.get(6), {6: 'six'});
	assert.end();
});

test('gc', function(assert) {
	var map = new WeakValueMap();
	let obj = {a: 1234, b: 'test', c: {d: 'testing'}};
	let obj1 = Object.assign({}, obj);
	let obj2 = Object.assign({}, obj);
	map.set(1, obj1);
	map.set(2, obj2);
	assert.deepEqual(map.get(1), obj);
	assert.deepEqual(map.get(2), obj);
	global.gc();
	assert.deepEqual(map.get(1), obj);
	assert.deepEqual(map.get(2), obj);
	obj2 = null;
	global.gc();
	assert.deepEqual(map.get(1), obj);
	assert.strictEqual(map.get(2), void 0);
	obj1 = null;
	global.gc();
	assert.strictEqual(map.get(1), void 0);
	assert.strictEqual(map.get(2), void 0);
	assert.end();
});