module.exports = {
  'account_guid': 'fxdkd0nakv',
  'username': 'username@oracle.com',
  'password': 'password',
  'streamUrl': 'wss://api.oracleinfinity.io/streaming',
  // 'root_cert': 'root.crt',
  'query': 'select ' +
			'data.movement,' +
      'data.action,' +
      'data.m_time,' +
      'data.p_time,' +
			'data.m_x,' +
      'data.m_y,' +
      'data.m_z,' +
      'data.velocity_x,' +
      'data.force,' +
      'data.rfid_tag,' +
      'data.rfid_tag_source,' +
      'data.team,' +
      'data.team_source,' +
      'data.player,' +
      'data.player_source',
};
