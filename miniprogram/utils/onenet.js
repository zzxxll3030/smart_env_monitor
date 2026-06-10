/**
 * OneNET HTTP API 通信模块
 * 使用 ESP32 预计算 token，无需 accessKey 签名
 */

const app = getApp();

// OneNET HTTP API 基础地址
const BASE_URL = 'https://iot-api.heclouds.com';

// ESP32 预计算 token（版本 2018-10-31，有效期到 2121 年）
const AUTH_TOKEN =
  'version=2018-10-31&res=products%2F6176tD4mc7%2Fdevices%2Fesp32&et=1812468109&method=md5&sign=zSAjzukJabfw%2Ft7tlF%2FtOg%3D%3D';

/**
 * 发起 OneNET HTTP 请求
 */
function request(method, urlPath, data) {
  return new Promise((resolve, reject) => {
    wx.request({
      url: BASE_URL + urlPath,
      method: method,
      header: {
        'Authorization': AUTH_TOKEN,
        'Content-Type': 'application/json'
      },
      data: data,
      success(res) {
        if (res.statusCode === 200) {
          resolve(res.data);
        } else {
          reject(new Error(`HTTP ${res.statusCode}: ${JSON.stringify(res.data)}`));
        }
      },
      fail(err) {
        reject(err);
      }
    });
  });
}

/**
 * 查询设备最新属性值（GET 请求，参数通过 URL query 传递）
 */
function queryDeviceProperties() {
  const g = app.globalData;
  const url = `${BASE_URL}/thingmodel/query-device-property?product_id=${g.productId}&device_name=${g.deviceName}`;
  return new Promise((resolve, reject) => {
    wx.request({
      url: url,
      method: 'GET',
      header: {
        'Authorization': AUTH_TOKEN
      },
      success(res) {
        if (res.statusCode === 200 && res.data && res.data.code === 0) {
          const props = {};
          (res.data.data || []).forEach(item => { props[item.identifier] = item.value; });
          resolve(props);
        } else {
          reject(new Error((res.data && res.data.msg) || '查询失败'));
        }
      },
      fail(err) { reject(err); }
    });
  });
}

/**
 * 设置设备属性（下发阈值命令 / 执行器开关）
 * @param {object} params - 如 { maxtemp_set: 40, led_switch: true }
 */
function setDeviceProperties(params) {
  const g = app.globalData;
  const body = {
    product_id: g.productId,
    device_name: g.deviceName,
    params: params
  };
  return request('POST', '/thingmodel/set-device-property', body).then(res => {
    if (res.code === 0) return res;
    throw new Error(res.msg || '下发失败');
  });
}

/**
 * 调用 OneNET 设备服务
 */
function invokeService(id, params) {
  const g = app.globalData;
  const body = {
    product_id: g.productId,
    device_name: g.deviceName,
    id: id,
    params: params
  };
  return request('POST', '/thingmodel/invoke-device-service', body).then(res => {
    if (res.code === 0) return res;
    throw new Error(res.msg || '调用失败');
  });
}

module.exports = {
  queryDeviceProperties,
  setDeviceProperties,
  invokeService
};
