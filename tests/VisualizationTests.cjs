const fs = require('node:fs');
const vm = require('node:vm');
const assert = require('node:assert/strict');

class Element {
  constructor(name) {
    this.name = name;
    this.attributes = {};
    this.children = [];
    this.style = {};
    this.events = new Map();
    this.value = '';
    this._text = '';
  }
  set textContent(value) { this._text = String(value); this.children = []; }
  get textContent() { return this._text + this.children.map(child => child.textContent).join(''); }
  setAttribute(name, value) { this.attributes[name] = String(value); }
  append(...children) { this.children.push(...children); }
  replaceChildren(...children) { this._text = ''; this.children = children; }
  addEventListener(name, handler) {
    if (!this.events.has(name)) this.events.set(name, []);
    this.events.get(name).push(handler);
  }
  fire(name) { for (const handler of this.events.get(name) || []) handler(); }
}

function escapeXml(text) {
  return String(text).replaceAll('&', '&amp;').replaceAll('<', '&lt;')
    .replaceAll('>', '&gt;').replaceAll('"', '&quot;');
}
function serialize(node) {
  const attributes = Object.entries(node.attributes).map(([name, value]) => ` ${name}="${escapeXml(value)}"`).join('');
  return `<${node.name}${attributes}>${escapeXml(node._text)}${node.children.map(serialize).join('')}</${node.name}>`;
}

assert(process.argv.length > 2, 'Pass at least one HTML report path');
for (const path of process.argv.slice(2)) {
  const html = fs.readFileSync(path, 'utf8');
  const script = html.match(/<script>([\s\S]*?)<\/script>/);
  assert(script, 'Report has no script');
  const elements = new Map();
  for (const match of html.matchAll(/<([a-z][a-z0-9]*)\b[^>]*\bid="([^"]+)"/g)) {
    elements.set(match[2], new Element(match[1]));
  }
  const get = id => {
    assert(elements.has(id), `Missing report element: ${id}`);
    return elements.get(id);
  };
  get('algorithm').value = 'two';
  const context = vm.createContext({
    document: {getElementById: get, createElementNS: (_, name) => new Element(name), createElement: name => new Element(name)},
    window: {scrollTo() {}}, console,
  });
  new vm.Script(script[1], {filename: path}).runInContext(context, {timeout: 5000});
  const data = vm.runInContext('data', context);
  if (data.vertices.length === data.n && data.edges.length === data.m) {
    const reference = Array(data.n + 1).fill(null);
    reference[data.source] = 0n;
    const pairs = new Set();
    for (const edge of data.edges) {
      assert(edge.from >= 1 && edge.from <= data.n && edge.to >= 1 && edge.to <= data.n);
      assert(BigInt(edge.weight) >= 0n);
      if (path.includes('random')) {
        assert.notEqual(edge.from, edge.to);
        assert(!pairs.has(edge.from + ':' + edge.to));
        assert(BigInt(edge.weight) >= 1n && BigInt(edge.weight) <= 100n);
      }
      pairs.add(edge.from + ':' + edge.to);
    }
    for (let step = 1; step < data.n; ++step) {
      for (const edge of data.edges) {
        if (reference[edge.from] === null) continue;
        const candidate = reference[edge.from] + BigInt(edge.weight);
        if (candidate < 9223372036854775807n && (reference[edge.to] === null || candidate < reference[edge.to])) {
          reference[edge.to] = candidate;
        }
      }
    }
    for (const name of ['two', 'fibonacci']) {
      for (let vertex = 1; vertex <= data.n; ++vertex) {
        assert.equal(data[name].dist[vertex], reference[vertex] === null ? null : reference[vertex].toString());
      }
    }
  }
  assert.equal(get('graph').children.filter(node => node.name === 'g').length, data.vertices.length);
  assert.equal(get('results').children.length, Math.min(100, data.n));
  assert(data.vertices.length <= 80 && data.vertices.includes(data.source));
  assert(data.edges.length <= 250);
  for (const name of ['two', 'fibonacci']) {
    const width = parseFloat(get('bar-' + name).style.width);
    assert(Number.isFinite(width) && width >= 0 && width <= 100);
    get('algorithm').value = name;
    const target = data[name].dist.findIndex((distance, vertex) => vertex > 0 && vertex !== data.source && distance !== null);
    get('target').value = target === -1 ? data.source : target;
    get('algorithm').fire('change');
    const selected = Number(get('target').value);
    assert(get('path').textContent.includes(data[name].dist[selected]));
    const expected = [];
    for (let vertex = selected; vertex; vertex = data[name].up[vertex]) expected.push(vertex);
    expected.reverse();
    assert(get('path').textContent.includes(expected.join(' \u2192 ')));
    const visiblePathEdges = data.edges.filter(edge => expected.some((vertex, i) =>
      i && expected[i - 1] === edge.from && vertex === edge.to &&
      BigInt(data[name].dist[edge.from]) + BigInt(edge.weight) === BigInt(data[name].dist[edge.to])));
    assert.equal(get('graph').children.filter(node => node.attributes.class === 'edge path').length, visiblePathEdges.length);
    const unreachable = data[name].dist.findIndex((distance, vertex) => vertex > 0 && distance === null);
    if (unreachable !== -1) {
      get('target').value = unreachable;
      get('algorithm').fire('change');
      assert(get('path').textContent.includes('недостижима'));
      assert.equal(get('graph').children.filter(node => node.attributes.class === 'edge path').length, 0);
      get('target').value = selected;
      get('algorithm').fire('change');
    }
  }
  if (data.n > 100) {
    get('next').fire('click');
    assert.equal(get('results').children[0].children[0].textContent, '101');
    get('previous').fire('click');
    assert.equal(get('results').children[0].children[0].textContent, '1');
  }
  const css = html.match(/<style>([\s\S]*?)<\/style>/)[1];
  const svg = get('graph');
  svg.setAttribute('xmlns', 'http://www.w3.org/2000/svg');
  svg.setAttribute('width', '800');
  svg.setAttribute('height', '620');
  svg.setAttribute('viewBox', '0 0 800 620');
  const style = new Element('style');
  style.textContent = css;
  svg.children.unshift(style);
  fs.writeFileSync(path + '.svg', serialize(svg));
  console.log(`Passed report controls and SVG: ${path}`);
}
