'use strict';
'require form';
'require view';

return view.extend({
	render: function() {
		var m, s, o;

		m = new form.Map('bbmd', _('BBMD Configuration'),
			_('Configure BACnet/SC BBMD daemon settings. Changes require a daemon restart.'));

		s = m.section(form.TypedSection, 'globals', _('Globals'));
		s.anonymous = true;
		s.description = _('BACnet device identity and network settings.');

		o = s.option(form.Value, 'device_instance', _('Device Instance'));
		o.datatype = 'uinteger';
		o.placeholder = '4194303';
		o.description = _('Unique BACnet device instance number (0-4194303).');

		o = s.option(form.Value, 'device_name', _('Device Name'));
		o.datatype = 'string';
		o.placeholder = 'OpenWrt Router';

		o = s.option(form.Value, 'vendor_id', _('Vendor ID'));
		o.datatype = 'uinteger';
		o.placeholder = '0';

		o = s.option(form.Value, 'model_name', _('Model Name'));
		o.datatype = 'string';
		o.placeholder = 'OpenWrt BBMD';

		o = s.option(form.Value, 'firmware_rev', _('Firmware Revision'));
		o.datatype = 'string';
		o.placeholder = '1.0.0';

		o = s.option(form.Value, 'network_number', _('Network Number'));
		o.datatype = 'uinteger';
		o.placeholder = '1';

		// --- Hub ---

		s = m.section(form.TypedSection, 'hub', _('Hub'));
		s.anonymous = true;
		s.description = _('BACnet/SC hub settings. The hub accepts incoming connections from nodes.');

		o = s.option(form.Flag, 'enabled', _('Enabled'));
		o.default = '0';

		o = s.option(form.Value, 'port', _('Port'));
		o.datatype = 'port';
		o.placeholder = '443';
		o.depends('enabled', '1');

		o = s.option(form.Value, 'tls_cert', _('TLS Certificate'));
		o.datatype = 'string';
		o.placeholder = '/etc/bbmd/certs/hub-cert.pem';
		o.depends('enabled', '1');

		o = s.option(form.Value, 'tls_key', _('TLS Key'));
		o.datatype = 'string';
		o.placeholder = '/etc/bbmd/certs/hub-key.pem';
		o.depends('enabled', '1');

		o = s.option(form.Value, 'ca_cert', _('CA Certificate'));
		o.datatype = 'string';
		o.placeholder = '/etc/bbmd/certs/ca-cert.pem';
		o.depends('enabled', '1');

		o = s.option(form.Value, 'max_nodes', _('Max Nodes'));
		o.datatype = 'uinteger';
		o.placeholder = '50';
		o.depends('enabled', '1');

		o = s.option(form.Value, 'keepalive_interval', _('Keepalive Interval (s)'));
		o.datatype = 'uinteger';
		o.placeholder = '30';
		o.depends('enabled', '1');

		o = s.option(form.Value, 'keepalive_timeout', _('Keepalive Timeout (s)'));
		o.datatype = 'uinteger';
		o.placeholder = '90';
		o.depends('enabled', '1');

		// --- Node ---

		s = m.section(form.TypedSection, 'node', _('Node'));
		s.anonymous = true;
		s.description = _('BACnet/SC node settings. The node connects to a hub.');

		o = s.option(form.Flag, 'enabled', _('Enabled'));
		o.default = '0';

		o = s.option(form.Value, 'primary_hub', _('Primary Hub URI'));
		o.datatype = 'string';
		o.placeholder = 'wss://hub.example.com/bacnet-sc';
		o.depends('enabled', '1');

		o = s.option(form.DynamicList, 'failover_hub', _('Failover Hubs'));
		o.datatype = 'string';
		o.placeholder = 'wss://hub2.example.com/bacnet-sc';
		o.depends('enabled', '1');

		o = s.option(form.Value, 'tls_cert', _('TLS Certificate'));
		o.datatype = 'string';
		o.placeholder = '/etc/bbmd/certs/node-cert.pem';
		o.depends('enabled', '1');

		o = s.option(form.Value, 'tls_key', _('TLS Key'));
		o.datatype = 'string';
		o.placeholder = '/etc/bbmd/certs/node-key.pem';
		o.depends('enabled', '1');

		o = s.option(form.Value, 'ca_cert', _('CA Certificate'));
		o.datatype = 'string';
		o.placeholder = '/etc/bbmd/certs/ca-cert.pem';
		o.depends('enabled', '1');

		o = s.option(form.Value, 'retry_interval', _('Retry Interval (s)'));
		o.datatype = 'uinteger';
		o.placeholder = '5';
		o.depends('enabled', '1');

		o = s.option(form.Value, 'max_retry_interval', _('Max Retry Interval (s)'));
		o.datatype = 'uinteger';
		o.placeholder = '60';
		o.depends('enabled', '1');

		// --- Telemetry ---

		s = m.section(form.TypedSection, 'telemetry', _('Telemetry'));
		s.anonymous = true;
		s.description = _('System telemetry reporting intervals.');

		o = s.option(form.Flag, 'enabled', _('Enabled'));
		o.default = '1';

		o = s.option(form.Value, 'cpu_interval', _('CPU Interval (s)'));
		o.datatype = 'uinteger';
		o.placeholder = '10';
		o.depends('enabled', '1');

		o = s.option(form.Value, 'memory_interval', _('Memory Interval (s)'));
		o.datatype = 'uinteger';
		o.placeholder = '10';
		o.depends('enabled', '1');

		o = s.option(form.Value, 'network_interval', _('Network Interval (s)'));
		o.datatype = 'uinteger';
		o.placeholder = '30';
		o.depends('enabled', '1');

		o = s.option(form.Value, 'uptime_interval', _('Uptime Interval (s)'));
		o.datatype = 'uinteger';
		o.placeholder = '60';
		o.depends('enabled', '1');

		// --- BBMD Bridge ---

		s = m.section(form.TypedSection, 'bbmd', _('BBMD Bridge'));
		s.anonymous = true;
		s.description = _('BACnet BBMD bridge for BACnet/IP to BACnet/SC routing.');

		o = s.option(form.Flag, 'enabled', _('Enabled'));
		o.default = '0';

		o = s.option(form.Value, 'port', _('Port'));
		o.datatype = 'port';
		o.placeholder = '47808';
		o.depends('enabled', '1');

		o = s.option(form.Value, 'lan_interface', _('LAN Interface'));
		o.datatype = 'string';
		o.placeholder = 'br-lan';
		o.depends('enabled', '1');

		// --- Logging ---

		s = m.section(form.TypedSection, 'logging', _('Logging'));
		s.anonymous = true;
		s.description = _('Daemon logging configuration.');

		o = s.option(form.ListValue, 'level', _('Log Level'));
		o.default = 'info';
		o.value('error', _('Error'));
		o.value('warn', _('Warning'));
		o.value('info', _('Info'));
		o.value('debug', _('Debug'));

		return m.render();
	}
});
