App({
  globalData: {
    // ====== OneNET 配置 ======
    productId: '6176tD4mc7',
    deviceName: 'esp32',
    // ====== 阈值默认值 ======
    threshold: {
      maxtemp_set: 35,
      minitemp_set: 0,
      maxhum_set: 80,
      minihum_set: 20,
      minlight_set: 20,
      neardist_set: 10
    },
    // 最新传感器数据
    latestData: null,
    // 执行器状态
    ledOn: false,
    servoOn: false,
    motorOn: false
  }
});
