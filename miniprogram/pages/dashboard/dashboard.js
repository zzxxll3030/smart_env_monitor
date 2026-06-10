const app = getApp();
const onenet = require('../../utils/onenet');

// 模拟数据（OneNET 未配置时使用）
const mockData = {
  temp_value: 26.5,
  humidity_value: 65,
  light_value: 42,
  dist_value: 8.2
};

function formatTime(ts) {
  const d = new Date(ts);
  const pad = n => ('0' + n).slice(-2);
  return pad(d.getHours()) + ':' + pad(d.getMinutes()) + ':' + pad(d.getSeconds());
}

Page({
  data: {
    timeText: '',
    lastUpdate: '--',
    loading: true,
    errorMsg: '',
    cards: []
  },

  onShow() {
    this.updateTime();
    this.loadData();
    // 每 2 秒自动刷新（设备端 ~500ms 上报一次）
    this.timer = setInterval(() => {
      this.updateTime();
      this.loadData();
    }, 1000);
  },

  onHide() {
    if (this.timer) clearInterval(this.timer);
  },

  updateTime() {
    const now = Date.now();
    this.setData({
      timeText: formatTime(now),
      lastUpdate: formatTime(now)
    });
  },

  async loadData() {
    try {
      let props;
      try {
        props = await onenet.queryDeviceProperties();
      } catch (e) {
        // OneNET 未配置或请求失败，使用模拟数据
        console.log('[Dashboard] 使用模拟数据:', e.message);
        props = mockData;
      }

      // 缓存到全局，供「我的」页面使用
      app.globalData.latestData = props;

      const cards = [
        {
          type: 'temp',
          label: '温度',
          value: Number(props.temp_value || 0).toFixed(1),
          unit: '°C',
          color: '#E74C3C',
          percent: Math.min((props.temp_value || 0) / 60 * 100, 100)
        },
        {
          type: 'humid',
          label: '湿度',
          value: Math.round(props.humidity_value || 0),
          unit: '%',
          color: '#00B4D8',
          percent: Math.min((props.humidity_value || 0), 100)
        },
        {
          type: 'light',
          label: '光照',
          value: Math.round(props.light_value || 0),
          unit: 'lux',
          color: '#F39C12',
          percent: Math.min((props.light_value || 0), 100)
        },
        {
          type: 'dist',
          label: '距离',
          value: Number(props.dist_value || 0).toFixed(1),
          unit: 'cm',
          color: '#2ECC71',
          percent: Math.min((props.dist_value || 0) / 20 * 100, 100)
        }
      ];

      this.setData({ cards, loading: false, errorMsg: '' });
    } catch (err) {
      console.error('[Dashboard] 加载失败:', err);
      this.setData({ loading: false, errorMsg: '数据加载失败' });
    }
  },

  onRefresh() {
    this.setData({ loading: true, errorMsg: '' });
    this.loadData();
  }
});
