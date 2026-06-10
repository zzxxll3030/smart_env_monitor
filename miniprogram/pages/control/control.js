// pages/control/control.js
const app = getApp();

Page({
  data: {
    ledOn: false,
    servoOn: false,
    motorOn: false
  },

  onShow() {
    // 从全局读取当前状态
    this.setData({
      ledOn: app.globalData.ledOn || false,
      servoOn: app.globalData.servoOn || false,
      motorOn: app.globalData.motorOn || false
    });
  },

  // 下发单属性到 OneNET
  sendProp(prop, val) {
    const onenet = require('../../utils/onenet');
    const params = {};
    params[prop] = val;
    onenet.setDeviceProperties(params).catch(err => {
      wx.showToast({ title: '下发失败', icon: 'none' });
      console.log('[Control] 下发' + prop + '失败:', err.message);
    });
  },

  onLedToggle(e) {
    const val = e.detail.value;
    this.setData({ ledOn: val });
    app.globalData.ledOn = val;
    this.sendProp('led_switch', val);
  },

  onServoToggle(e) {
    const val = e.detail.value;
    this.setData({ servoOn: val });
    app.globalData.servoOn = val;
    this.sendProp('servo_switch', val);
  },

  onMotorToggle(e) {
    const val = e.detail.value;
    this.setData({ motorOn: val });
    app.globalData.motorOn = val;
    this.sendProp('motor_switch', val);
  }
});
