/**
 * 生成 tabBar 所需的 40x40 纯色占位图标（合法 PNG）
 * 后续可替换为设计师提供的正式图标
 */
const fs = require('fs');
const path = require('path');
const zlib = require('zlib');

const OUT = path.resolve(__dirname, '..', 'images');

function createPNG(width, height, fillRGBA) {
  // 逐行构造 raw 像素数据 (RGBA), filter byte = 0 (None)
  const rawRows = [];
  for (let y = 0; y < height; y++) {
    const row = Buffer.alloc(1 + width * 4);
    row[0] = 0; // filter: None
    for (let x = 0; x < width; x++) {
      const off = 1 + x * 4;
      row[off] = fillRGBA[0];     // R
      row[off + 1] = fillRGBA[1]; // G
      row[off + 2] = fillRGBA[2]; // B
      row[off + 3] = fillRGBA[3]; // A
    }
    rawRows.push(row);
  }
  const raw = Buffer.concat(rawRows);
  const compressed = zlib.deflateSync(raw);

  // --- 组装 PNG ---
  function chunk(type, data) {
    const len = Buffer.alloc(4);
    len.writeUInt32BE(data.length, 0);
    const typeB = Buffer.from(type, 'ascii');
    const crcInput = Buffer.concat([typeB, data]);
    // CRC-32
    let crc = 0xFFFFFFFF;
    for (let i = 0; i < crcInput.length; i++) {
      crc ^= crcInput[i];
      for (let j = 0; j < 8; j++) {
        if (crc & 1) crc = (crc >>> 1) ^ 0xEDB88320;
        else crc >>>= 1;
      }
    }
    crc = (crc ^ 0xFFFFFFFF) >>> 0;
    const crcB = Buffer.alloc(4);
    crcB.writeUInt32BE(crc, 0);
    return Buffer.concat([len, typeB, data, crcB]);
  }

  const signature = Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]);

  const ihdrData = Buffer.alloc(13);
  ihdrData.writeUInt32BE(width, 0);
  ihdrData.writeUInt32BE(height, 4);
  ihdrData[8] = 8;  // bit depth
  ihdrData[9] = 6;  // color type: RGBA
  ihdrData[10] = 0; // compression
  ihdrData[11] = 0; // filter
  ihdrData[12] = 0; // interlace

  return Buffer.concat([
    signature,
    chunk('IHDR', ihdrData),
    chunk('IDAT', compressed),
    chunk('IEND', Buffer.alloc(0)),
  ]);
}

const SIZE = 40;

const icons = [
  //          name            normal RGBA          active RGBA
  ['dashboard',       [100, 140, 160, 255],  [0,   119, 182, 255]], // 灰蓝 → 深蓝
  ['control',         [100, 140, 160, 255],  [0,   119, 182, 255]], // 灰蓝 → 深蓝
  ['threshold',       [100, 140, 160, 255],  [0,   119, 182, 255]], // 灰蓝 → 深蓝
  ['history',         [100, 140, 160, 255],  [0,   119, 182, 255]], // 灰蓝 → 深蓝
];

if (!fs.existsSync(OUT)) fs.mkdirSync(OUT, { recursive: true });

for (const [name, normal, active] of icons) {
  fs.writeFileSync(path.join(OUT, `${name}.png`), createPNG(SIZE, SIZE, normal));
  fs.writeFileSync(path.join(OUT, `${name}-active.png`), createPNG(SIZE, SIZE, active));
  console.log(`✓ ${name}.png / ${name}-active.png`);
}

console.log('\n所有图标已生成。后续请替换为正式设计图标（40×40 PNG）。');
