#pragma once

#include "Dijkstra.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <locale>
#include <stdexcept>
#include <string>
#include <vector>

struct BenchmarkPoint {
    int n;
    std::size_t m;
    double timeTwo;
    double timeFibonacci;
};

class Visualizer {
    static void writeResult(std::ostream& output, const DijkstraResult& result) {
        output << "{\"dist\":[";
        for (std::size_t i = 0; i < result.dist.size(); ++i) {
            if (i != 0) output << ',';
            if (result.dist[i] == INT64_MAX) output << "null";
            else output << '"' << result.dist[i] << '"';
        }
        output << "],\"up\":[";
        for (std::size_t i = 0; i < result.up.size(); ++i) {
            if (i != 0) output << ',';
            output << result.up[i];
        }
        output << "]}";
    }

public:
    void saveBenchmarkHtml(const std::string& path,
                           const std::vector<BenchmarkPoint>& sparse,
                           const std::vector<BenchmarkPoint>& medium,
                           const std::vector<BenchmarkPoint>& dense,
                           const std::vector<BenchmarkPoint>& manyDecreases,
                           int repeats, long long minWeight, long long maxWeight) const {
        if (sparse.empty() || medium.empty() || dense.empty() || manyDecreases.empty())
            throw std::invalid_argument("Empty benchmark results");
        std::ofstream output(path);
        if (!output) throw std::runtime_error("Cannot open benchmark report: " + path);
        output.imbue(std::locale::classic());
        output << std::setprecision(17);
        output << R"HTML(<!doctype html><html lang="ru"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Сравнение Дейкстры</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js@4.6.1/dist/chart.umd.min.js"></script>
<style>body{font:16px Segoe UI,Arial,sans-serif;max-width:1000px;margin:32px auto;padding:0 20px;color:#192333;background:#f3f5f8}section{background:white;padding:24px;border-radius:12px;margin:24px 0}.chart{height:360px}h2{font-size:20px}p{line-height:1.5}</style>
</head><body><h1>Сравнение двух реализаций Дейкстры</h1><p>)HTML";
        output << "Случайные графы: веса от " << minWeight << " до " << maxWeight
               << ". Источник 1, seed 42. Медиана по "
               << repeats << " запускам после прогрева каждого алгоритма. Генерация графа не входит в замер.</p>";
        output << R"HTML(<p id="status"></p>
<section><h2>Разреженные графы: m = 4n</h2><div class="chart"><canvas id="sparse"></canvas></div></section>
<section><h2>Средние графы: случайное m между 4n и ⌊n(n−1)/4⌋</h2><div class="chart"><canvas id="medium"></canvas></div></section>
<section><h2>Плотные графы: m = ⌊n(n−1)/4⌋</h2><div class="chart"><canvas id="dense"></canvas></div></section>
<section><h2>Специальные графы с частыми уменьшениями ключа</h2><p>Веса и порядок рёбер заданы конструкцией стресс-теста.</p><div class="chart"><canvas id="manyDecreases"></canvas></div></section>
<script>
)HTML";
        auto writePoints = [&](const char* name, const std::vector<BenchmarkPoint>& points) {
            output << "const " << name << " = [";
            for (std::size_t i = 0; i < points.size(); ++i) {
                if (i != 0) output << ',';
                const auto& point = points[i];
                output << "{\"n\":" << point.n << ",\"m\":" << point.m << ",\"two\":" << point.timeTwo
                       << ",\"fibonacci\":" << point.timeFibonacci << '}';
            }
            output << "];\n";
        };
        writePoints("sparse", sparse);
        writePoints("medium", medium);
        writePoints("dense", dense);
        writePoints("manyDecreases", manyDecreases);
        output << R"HTML(
function draw(id, points) {
  new Chart(document.getElementById(id), {
    type: 'line',
    data: {datasets: [['two','2-куча','#315bc7'], ['fibonacci','Куча Фибоначчи','#14825d']].map(([key,label,color]) => ({
      label, borderColor:color, backgroundColor:color, pointRadius:4, tension:0,
      data:points.map(point => ({x:point.n, y:point[key]*1000, m:point.m}))
    }))},
    options: {
      responsive:true, maintainAspectRatio:false, animation:false,
      interaction:{mode:'index', intersect:false},
      scales:{x:{type:'linear', title:{display:true,text:'Количество вершин n'}, ticks:{precision:0}},
              y:{beginAtZero:true,title:{display:true,text:'Время, мс'}}},
      plugins:{tooltip:{callbacks:{afterLabel:context => `n=${context.raw.x}, m=${context.raw.m}`}}}
    }
  });
}
if (typeof Chart === 'undefined') {
  document.getElementById('status').textContent = 'Не удалось загрузить Chart.js. Для просмотра графиков требуется интернет.';
} else {
  draw('sparse', sparse);
  draw('medium', medium);
  draw('dense', dense);
  draw('manyDecreases', manyDecreases);
}
</script></body></html>)HTML";
        output.flush();
        if (!output) throw std::runtime_error("Cannot write benchmark report: " + path);
    }
    void saveHtml(const std::string& path, const AdjacencyList& adjacency, int source,
                  const DijkstraResult& two, double timeTwo,
                  const DijkstraResult& fibonacci, double timeFibonacci) const {
        if (adjacency.size() < 2 || source < 1 || static_cast<std::size_t>(source) >= adjacency.size() ||
            two.dist.size() != adjacency.size() || two.up.size() != adjacency.size() ||
            fibonacci.dist.size() != adjacency.size() || fibonacci.up.size() != adjacency.size() ||
            !std::isfinite(timeTwo) || !std::isfinite(timeFibonacci) || timeTwo < 0 || timeFibonacci < 0) {
            throw std::invalid_argument("Invalid visualization data");
        }
        std::ofstream output(path);
        if (!output) throw std::runtime_error("Cannot open visualization file: " + path);
        output.imbue(std::locale::classic());
        output << std::setprecision(17);
        const int n = static_cast<int>(adjacency.size()) - 1;
        std::size_t m = 0;
        for (int vertex = 1; vertex <= n; ++vertex) m += adjacency[vertex].size();
        std::vector<int> vertices{source};
        for (int vertex = 1; vertex <= n && vertices.size() < 80; ++vertex) {
            if (vertex != source) vertices.push_back(vertex);
        }
        std::vector<bool> visible(adjacency.size(), false);
        for (int vertex : vertices) visible[vertex] = true;

        output << R"HTML(<!doctype html>
<html lang="ru"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Dijkstra — граф и время работы</title>
<style>
:root{font-family:Segoe UI,Arial,sans-serif;color:#192333;background:#f3f5f8}*{box-sizing:border-box}
body{max-width:1280px;margin:auto;padding:32px 24px}h1{font-size:30px;margin:0 0 8px}h2{font-size:19px;margin:0 0 20px}
p{line-height:1.5}.muted{color:#586779;font-size:14px}.grid{display:grid;grid-template-columns:minmax(0,2fr) minmax(280px,1fr);gap:20px}
.panel{background:white;padding:24px;border:1px solid #dce3ed;border-radius:14px;margin:20px 0}
label{display:inline-flex;align-items:center;gap:8px;margin:0 12px 10px 0;font-size:14px}
select,input,button{font:inherit;padding:7px 10px;border:1px solid #bfcbd9;border-radius:6px;background:white;color:inherit}
input{width:100px}button{cursor:pointer}button:disabled{opacity:.45;cursor:default}svg{display:block;width:100%;height:auto}
.edge{fill:none;stroke:#c0cad7;stroke-width:1.2}.edge.path{stroke:#14825d;stroke-width:3}.weight{font-size:11px;fill:#5b6878;paint-order:stroke;stroke:white;stroke-width:3px}
.node circle{fill:#edf2f8;stroke:#718197;stroke-width:1.5}.node.unreachable circle{fill:#fafafa;stroke:#c6cdd5;stroke-dasharray:3 2}
.node.target circle{fill:#f7ae48;stroke:#9c6019}.node.source circle{fill:#315bc7;stroke:#2346a0}.node text{font-size:12px;text-anchor:middle;dominant-baseline:central}
.node.source text{fill:white}.bar{height:26px;border-radius:4px;background:#315bc7;min-width:0}.bar.fibonacci{background:#14825d}
.track{background:#edf1f6;border-radius:4px;overflow:hidden;margin:8px 0 20px}.metric{display:flex;justify-content:space-between;gap:12px;font-size:14px}
table{width:100%;border-collapse:collapse;font-size:14px}th,td{text-align:left;padding:10px;border-bottom:1px solid #e6ebf1}th{color:#566376}
.table-wrap{overflow:auto}.path-text{overflow-wrap:anywhere}#page{display:inline-block;margin:0 12px}
@media(max-width:850px){.grid{display:block}body{padding:20px 12px}.panel{padding:18px}h1{font-size:25px}}
</style></head><body>
<h1>Дейкстра: граф и время работы</h1><p class="muted" id="summary"></p>
<div class="grid"><section class="panel"><h2>Граф и кратчайший путь</h2>
<label>Алгоритм <select id="algorithm"><option value="two">2-куча</option><option value="fibonacci">Куча Фибоначчи</option></select></label>
<label>Цель <input id="target" type="number" min="1" step="1"></label><button id="show" type="button">Показать путь</button>
<svg id="graph" viewBox="0 0 800 620" role="img" aria-label="Граф и выбранный кратчайший путь"></svg>
<p id="drawing" class="muted"></p><p id="path" class="path-text"></p>
<p class="muted">Синий узел — источник, оранжевый — цель, зелёные рёбра — кратчайший путь. Вес указан у ребра; полное значение доступно при наведении.</p>
</section><aside class="panel"><h2>Время одного запуска</h2>
<div class="metric"><span>2-куча</span><strong id="time-two"></strong></div><div class="track"><div class="bar" id="bar-two"></div></div>
<div class="metric"><span>Куча Фибоначчи</span><strong id="time-fibonacci"></strong></div><div class="track"><div class="bar fibonacci" id="bar-fibonacci"></div></div>
<p class="muted">Шкала общая, от нуля до большего времени. Замер включает инициализацию и работу алгоритма; генерация, чтение и визуализация выполняются отдельно.</p>
<p class="muted">2-куча: O((n + m) log n).<br>Куча Фибоначчи: O(m + n log n) амортизированно.</p>
</aside></div>
<section class="panel"><h2>Расстояния и предшественники</h2><div class="table-wrap"><table>
<thead><tr><th>Вершина</th><th>Расстояние</th><th>Предшественник: 2-куча</th><th>Предшественник: Фибоначчи</th></tr></thead><tbody id="results"></tbody></table></div>
<p><button id="previous" type="button">Назад</button><span id="page"></span><button id="next" type="button">Далее</button></p>
</section><script>
const data = )HTML";
        output << "{\"n\":" << n << ",\"m\":" << m << ",\"source\":" << source
               << ",\"timeTwo\":" << timeTwo << ",\"timeFibonacci\":" << timeFibonacci << ",\"vertices\":[";
        for (std::size_t i = 0; i < vertices.size(); ++i) {
            if (i != 0) output << ',';
            output << vertices[i];
        }
        output << "],\"edges\":[";
        std::size_t drawn = 0;
        for (int from : vertices) {
            for (const auto& [to, weight] : adjacency[from]) {
                if (!visible[to] || drawn >= 250) continue;
                if (drawn++ != 0) output << ',';
                output << "{\"from\":" << from << ",\"to\":" << to << ",\"weight\":\"" << weight << "\"}";
            }
        }
        output << "],\"two\":";
        writeResult(output, two);
        output << ",\"fibonacci\":";
        writeResult(output, fibonacci);
        output << R"HTML(};
const $ = id => document.getElementById(id);
const svgNS = 'http://www.w3.org/2000/svg';
const element = (name, attributes = {}, text) => {
  const node = document.createElementNS(svgNS, name);
  for (const [key, value] of Object.entries(attributes)) node.setAttribute(key, value);
  if (text !== undefined) node.textContent = text;
  return node;
};
$('summary').textContent = `${data.n} вершин · ${data.m} ориентированных рёбер · источник: ${data.source}`;
$('target').max = data.n;
$('target').value = data.vertices.find(vertex => vertex !== data.source) || data.source;
const formatTime = seconds => seconds < 0.001 ? `${(seconds * 1e6).toFixed(3)} мкс` : `${(seconds * 1000).toFixed(3)} мс`;
const maximum = Math.max(data.timeTwo, data.timeFibonacci);
for (const [name, value] of [['two', data.timeTwo], ['fibonacci', data.timeFibonacci]]) {
  $('time-' + name).textContent = formatTime(value);
  $('bar-' + name).style.width = (maximum ? value / maximum * 100 : 0) + '%';
}
$('drawing').textContent = `Показано ${data.vertices.length} из ${data.n} вершин и ${data.edges.length} из ${data.m} рёбер. Расчёт и таблица включают весь граф.`;
const positions = new Map([[data.source, {x:400, y:310}]]);
const others = data.vertices.filter(vertex => vertex !== data.source);
others.forEach((vertex, index) => {
  const angle = 2 * Math.PI * index / Math.max(1, others.length) - Math.PI / 2;
  positions.set(vertex, {x:400 + 335 * Math.cos(angle), y:310 + 250 * Math.sin(angle)});
});
function draw() {
  const target = Number($('target').value);
  if (!Number.isInteger(target) || target < 1 || target > data.n) {
    $('path').textContent = `Укажите вершину от 1 до ${data.n}.`;
    return;
  }
  const result = data[$('algorithm').value];
  const path = [], pathEdges = new Set();
  if (result.dist[target] !== null) {
    let current = target;
    for (let steps = 0; current && steps <= data.n; ++steps) {
      path.push(current);
      const parent = result.up[current];
      if (parent) pathEdges.add(parent + ':' + current);
      current = parent;
    }
    path.reverse();
  }
  $('path').textContent = path.length ? `Путь: ${path.join(' → ')}. Расстояние: ${result.dist[target]}.` : 'Вершина недостижима из источника.';
  if (path.length && path.some(vertex => !positions.has(vertex))) $('path').textContent += ' Часть вершин пути находится за пределами рисунка.';
  const svg = $('graph');
  svg.replaceChildren();
  const defs = element('defs');
  for (const [id, color] of [['arrow', '#aab7c6'], ['arrow-path', '#14825d']]) {
    const marker = element('marker', {id, viewBox:'0 0 10 10', refX:9, refY:5, markerWidth:7, markerHeight:7, orient:'auto'});
    marker.append(element('path', {d:'M 0 0 L 10 5 L 0 10 z', fill:color}));
    defs.append(marker);
  }
  svg.append(defs);
  const counts = new Map();
  for (const edge of data.edges) {
    const from = positions.get(edge.from), to = positions.get(edge.to);
    const pair = edge.from + ':' + edge.to;
    const occurrence = counts.get(pair) || 0;
    counts.set(pair, occurrence + 1);
    const highlighted = pathEdges.has(pair) && result.dist[edge.from] !== null &&
      BigInt(result.dist[edge.from]) + BigInt(edge.weight) === BigInt(result.dist[edge.to]);
    let curve, label;
    if (edge.from === edge.to) {
      const radius = 32 + occurrence * 14;
      curve = `M ${from.x-9} ${from.y-9} C ${from.x-radius*2} ${from.y-radius*2}, ${from.x+radius*2} ${from.y-radius*2}, ${from.x+9} ${from.y-9}`;
      label = {x:from.x, y:from.y-radius*1.5};
    } else {
      const dx = to.x-from.x, dy = to.y-from.y, length = Math.hypot(dx,dy);
      const offset = 18 + occurrence * 20;
      const control = {x:(from.x+to.x)/2-dy/length*offset, y:(from.y+to.y)/2+dx/length*offset};
      curve = `M ${from.x+dx/length*15} ${from.y+dy/length*15} Q ${control.x} ${control.y} ${to.x-dx/length*18} ${to.y-dy/length*18}`;
      label = {x:(from.x+to.x+2*control.x)/4, y:(from.y+to.y+2*control.y)/4-4};
    }
    const line = element('path', {d:curve, class:'edge'+(highlighted?' path':''), 'marker-end':`url(#${highlighted?'arrow-path':'arrow'})`});
    line.append(element('title', {}, `${edge.from} → ${edge.to}, вес ${edge.weight}`));
    svg.append(line);
    const caption = edge.weight.length > 11 ? edge.weight.slice(0,8)+'…' : edge.weight;
    const text = element('text', {...label, class:'weight', 'text-anchor':'middle'}, caption);
    text.append(element('title', {}, edge.weight));
    svg.append(text);
  }
  for (const vertex of data.vertices) {
    const position = positions.get(vertex);
    const classes = 'node'+(result.dist[vertex]===null?' unreachable':'')+(vertex===target?' target':'')+(vertex===data.source?' source':'');
    const node = element('g', {class:classes});
    node.append(element('circle', {cx:position.x, cy:position.y, r:14}));
    node.append(element('text', {x:position.x, y:position.y}, vertex));
    node.append(element('title', {}, `Вершина ${vertex}, расстояние ${result.dist[vertex]??'INF'}`));
    node.style.cursor = 'pointer';
    node.addEventListener('click', () => {$('target').value=vertex; draw();});
    svg.append(node);
  }
}
let page = 0;
const pageSize = 100;
function table() {
  const first = page * pageSize + 1, last = Math.min(data.n, first + pageSize - 1);
  $('results').replaceChildren();
  for (let vertex=first; vertex<=last; ++vertex) {
    const row = document.createElement('tr');
    for (const value of [vertex, data.two.dist[vertex]??'INF', data.two.up[vertex]||'—', data.fibonacci.up[vertex]||'—']) {
      const cell = document.createElement('td'); cell.textContent=value; row.append(cell);
    }
    row.style.cursor='pointer';
    row.addEventListener('click', () => {$('target').value=vertex; draw(); window.scrollTo({top:0,behavior:'smooth'});});
    $('results').append(row);
  }
  $('page').textContent=`${first}–${last} из ${data.n}`;
  $('previous').disabled=page===0;
  $('next').disabled=last===data.n;
}
$('show').addEventListener('click', draw);
$('algorithm').addEventListener('change', draw);
$('target').addEventListener('change', draw);
$('previous').addEventListener('click', () => {--page;table();});
$('next').addEventListener('click', () => {++page;table();});
draw();table();
</script></body></html>)HTML";
        output.flush();
        if (!output) throw std::runtime_error("Cannot write visualization file: " + path);
    }
};

