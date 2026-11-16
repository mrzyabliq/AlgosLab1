const fs = require('node:fs');
const assert = require('node:assert/strict');
const vm = require('node:vm');
const html = fs.readFileSync(process.argv[2] || 'benchmark.html', 'utf8');
const csv = fs.readFileSync(process.argv[3] || 'benchmark_samples.csv', 'utf8');
const count = Number(html.match(/Медиана по (\d+) запускам/)[1]);
assert(count >= 2 && count % 2 === 0, 'Use a balanced number of measured runs');
const [header, ...lines] = csv.trim().split(/\r?\n/);
assert.equal(header, 'type,n,m,repeat,first,two_seconds,fibonacci_seconds');
const records = lines.map(line => {
    const [type,n,m,repeat,first,two,fibonacci] = line.split(',');
    return {type,n:Number(n),m:Number(m),repeat:Number(repeat),first,two:Number(two),fibonacci:Number(fibonacci)};
});
const types = {Sparse:'sparse',Medium:'medium',Dense:'dense',ManyDecreases:'manyDecreases'};
const drawn = new Map();
new vm.Script(html.match(/<script>([\s\S]*?)<\/script>/)[1]).runInNewContext({
    Chart:function(element,config){drawn.set(element.id,config);},
    document:{getElementById:id=>({id})}
});
assert.equal(drawn.size,4);
const median = values => {
    values.sort((a,b) => a-b);
    const middle = values.length/2;
    return (values[middle-1]+values[middle])/2;
};
let points = 0;
for (const [type,name] of Object.entries(types)) {
    const series = JSON.parse(html.match(new RegExp('const '+name+' = (\\[[^\\r\\n]*\\]);'))[1]);
    let previous = 0;
    const chart = drawn.get(name);
    assert.equal(chart.data.datasets.length,2);
    for (const [index,point] of series.entries()) {
        assert(point.n > previous, 'Vertex counts must increase'); previous = point.n;
        const runs = records.filter(run => run.type === type && run.n === point.n);
        assert.equal(runs.length, count);
        assert.equal(new Set(runs.map(run => run.repeat)).size, count);
        assert.equal(runs.filter(run => run.first === 'two').length, count/2);
        for (const run of runs) {
            assert.equal(run.m, point.m);
            assert(run.repeat >= 1 && run.repeat <= count);
            assert.equal(run.first, run.repeat % 2 ? 'two' : 'fibonacci');
            assert(Number.isFinite(run.two) && run.two > 0);
            assert(Number.isFinite(run.fibonacci) && run.fibonacci > 0);
        }
        assert(Math.abs(point.two - median(runs.map(run => run.two))) < 1e-12);
        assert(Math.abs(point.fibonacci - median(runs.map(run => run.fibonacci))) < 1e-12);
        for (const [dataset,key] of [[0,'two'],[1,'fibonacci']]) {
            const plotted = chart.data.datasets[dataset].data[index];
            assert.equal(plotted.x,point.n);
            assert.equal(plotted.m,point.m);
            assert.equal(plotted.y,point[key]*1000);
        }
        ++points;
    }
}
assert.equal(records.length, points*count, 'Unexpected extra measurements');
console.log(`Passed: ${points} plotted points exactly match medians of ${records.length} raw runs; ordering, units and run balance verified.`);
