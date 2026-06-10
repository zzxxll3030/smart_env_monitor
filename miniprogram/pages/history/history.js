const app = getApp();
const onenet = require('../../utils/onenet');

// 阈值字段映射
const THR_MAP = [
  { key: 'maxtemp_set',  label: '温度上限', unit: '°C', icon: '🔥' },
  { key: 'minitemp_set', label: '温度下限', unit: '°C', icon: '❄️' },
  { key: 'maxhum_set',   label: '湿度上限', unit: '%',  icon: '💧' },
  { key: 'minihum_set',  label: '湿度下限', unit: '%',  icon: '🌵' },
  { key: 'minlight_set', label: '光照下限', unit: '%',  icon: '☀️' },
  { key: 'neardist_set', label: '接近距离', unit: 'cm', icon: '📏' }
];

Page({
  data: {
    // 传感器数据
    temp: '--',
    humidity: '--',
    light: '--',
    distance: '--',
    updateTime: '',

    // 执行器状态
    ledOn: false,
    servoOn: false,
    motorOn: false,

    // 阈值
    thresholds: [],

    // 设备信息
    productId: '',
    deviceName: ''
  },

  onShow() {
    this.loadDeviceInfo();
    this.syncActuatorStatus();
    this.syncThresholds();
    this.loadSensorData();
  },

  loadDeviceInfo() {
    this.setData({
      productId: app.globalData.productId,
      deviceName: app.globalData.deviceName
    });
  },

  syncActuatorStatus() {
    this.setData({
      ledOn: app.globalData.ledOn || false,
      servoOn: app.globalData.servoOn || false,
      motorOn: app.globalData.motorOn || false
    });
  },

  syncThresholds() {
    const t = app.globalData.threshold;
    const thresholds = THR_MAP.map(item => ({
      ...item,
      value: t[item.key]
    }));
    this.setData({ thresholds });
  },

  async loadSensorData() {
    try {
      const props = await onenet.queryDeviceProperties();
      // 写入全局缓存供其他页面使用
      app.globalData.latestData = props;
      this.setData({
        temp: Number(props.temp_value || 0).toFixed(1),
        humidity: Math.round(props.humidity_value || 0),
        light: Math.round(props.light_value || 0),
        distance: Number(props.dist_value || 0).toFixed(1),
        updateTime: this.nowStr()
      });
    } catch (e) {
      console.log('[Mine] 传感器数据加载失败:', e.message);
      // 尝试用缓存数据
      const cached = app.globalData.latestData;
      if (cached) {
        this.setData({
          temp: Number(cached.temp_value || 0).toFixed(1),
          humidity: Math.round(cached.humidity_value || 0),
          light: Math.round(cached.light_value || 0),
          distance: Number(cached.dist_value || 0).toFixed(1)
        });
      }
    }
  },

  nowStr() {
    const d = new Date();
    const pad = n => ('0' + n).slice(-2);
    return `${pad(d.getHours())}:${pad(d.getMinutes())}:${pad(d.getSeconds())}`;
  }
});
