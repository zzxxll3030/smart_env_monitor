const app = getApp();
const onenet = require('../../utils/onenet');

Page({
  data: {
    temp_max: 35,
    temp_min: 0,
    hum_max: 80,
    hum_min: 20,
    light_min: 20,
    dist_near: 10,
    saving: false,
    saved: false
  },

  onShow() {
    this.syncFromGlobal();
  },

  // 从 app.globalData.threshold 同步阈值
  syncFromGlobal() {
    const t = app.globalData.threshold;
    this.setData({
      temp_max: t.maxtemp_set,
      temp_min: t.minitemp_set,
      hum_max: t.maxhum_set,
      hum_min: t.minihum_set,
      light_min: t.minlight_set,
      dist_near: t.neardist_set
    });
  },

  onTempMax(e)  { this.setData({ temp_max: e.detail.value, saved: false }); },
  onTempMin(e)  { this.setData({ temp_min: e.detail.value, saved: false }); },
  onHumMax(e)   { this.setData({ hum_max: e.detail.value, saved: false }); },
  onHumMin(e)   { this.setData({ hum_min: e.detail.value, saved: false }); },
  onLightMin(e) { this.setData({ light_min: e.detail.value, saved: false }); },
  onDistNear(e) { this.setData({ dist_near: e.detail.value, saved: false }); },

  // 保存并下发
  async onSave() {
    this.setData({ saving: true });

    const params = {
      maxtemp_set: parseFloat(this.data.temp_max),
      minitemp_set: parseFloat(this.data.temp_min),
      maxhum_set: parseFloat(this.data.hum_max),
      minihum_set: parseFloat(this.data.hum_min),
      minlight_set: parseInt(this.data.light_min),
      neardist_set: parseFloat(this.data.dist_near)
    };

    // 更新本地
    app.globalData.threshold = { ...params };

    try {
      // 通过 OneNET HTTP API 下发
      await onenet.setDeviceProperties({
        maxtemp_set: parseFloat(this.data.temp_max),
        minitemp_set: parseFloat(this.data.temp_min),
        maxhum_set: parseFloat(this.data.hum_max),
        minihum_set: parseFloat(this.data.hum_min),
        minlight_set: parseInt(this.data.light_min),
        neardist_set: parseFloat(this.data.dist_near)
      });
      wx.showToast({ title: '下发成功', icon: 'success' });
    } catch (err) {
      console.log('[Threshold] OneNET 下发失败:', err.message);
      // OneNET 未配置时仍标记本地保存成功
      wx.showToast({ title: '已保存（离线）', icon: 'none' });
    }

    this.setData({ saving: false, saved: true });
    setTimeout(() => this.setData({ saved: false }), 2500);
  }
});
