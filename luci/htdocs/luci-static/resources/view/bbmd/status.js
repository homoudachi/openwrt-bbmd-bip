'use strict';
'require view';
'require fs';
'require ui';
'require uci';

var formatUptime = function(seconds) {
	var s = Math.floor(parseFloat(seconds) || 0);
	var d = Math.floor(s / 86400);
	var h = Math.floor((s % 86400) / 3600);
	var m = Math.floor((s % 3600) / 60);
	var sec = s % 60;
	var parts = [];
	if (d > 0) parts.push(d + 'd');
	if (h > 0) parts.push(h + 'h');
	if (m > 0) parts.push(m + 'm');
	parts.push(sec + 's');
	return parts.join(' ');
};

var fmtBytes = function(kb) {
	var k = parseInt(kb, 10);
	if (isNaN(k)) return '-';
	if (k >= 1048576) return (k / 1048576).toFixed(2) + ' GB';
	if (k >= 1024) return (k / 1024).toFixed(2) + ' MB';
	return k + ' KB';
};

var field = function(label, value) {
	var valCell = E('td', { 'class': 'cbi-value-field' });
	if (value !== null && typeof value === 'object' && value.nodeType) {
		valCell.appendChild(value);
	} else {
		valCell.appendChild(E('div', {}, value != null ? '' + value : '-'));
	}
	return E('tr', { 'class': 'cbi-section-table-row' }, [
		E('td', { 'class': 'cbi-value-name', 'style': 'width:280px;vertical-align:top' }, label),
		valCell
	]);
};

return view.extend({
	load: function() {
		return Promise.all([
			L.resolveDefault(fs.stat('/var/run/bbmd.pid'), null),
			L.resolveDefault(fs.read('/proc/uptime'), ''),
			L.resolveDefault(fs.read('/proc/meminfo'), ''),
			L.resolveDefault(fs.read('/proc/stat'), ''),
			uci.load('bbmd')
		]);
	},

	render: function(data) {
		var pidStat = data[0];
		var uptimeRaw = data[1];
		var meminfoRaw = data[2];
		var statRaw = data[3];

		var processRunning = pidStat && pidStat.type === 'file';

		var uciGet = function(section, option, def) {
			return uci.get('bbmd', section, option) || def;
		};

		var globalsSection = 'globals';
		var hubSection = 'hub';
		var nodeSection = 'node';
		var telemetrySection = 'telemetry';
		var bbmdSection = 'bbmd';
		var loggingSection = 'logging';

		var hubEnabled = uciGet(hubSection, 'enabled', '0') === '1';
		var nodeEnabled = uciGet(nodeSection, 'enabled', '0') === '1';
		var telemetryEnabled = uciGet(telemetrySection, 'enabled', '0') === '1';
		var bbmdEnabled = uciGet(bbmdSection, 'enabled', '0') === '1';

		var uptimeSecs = parseFloat((uptimeRaw || '').split(' ')[0]) || 0;
		var uptimeStr = formatUptime(uptimeSecs);

		var memTotal = 0, memAvailable = 0;
		if (meminfoRaw) {
			var mt = meminfoRaw.match(/MemTotal:\s+(\d+)/);
			var ma = meminfoRaw.match(/MemAvailable:\s+(\d+)/);
			if (mt) memTotal = parseInt(mt[1], 10);
			if (ma) memAvailable = parseInt(ma[1], 10);
		}
		var memUsed = memTotal - memAvailable;
		var memPct = memTotal > 0 ? ((memUsed / memTotal) * 100).toFixed(1) : '?';

		var cpuUsedPct = '?', cpuIdlePct = '?';
		if (statRaw) {
			var cpuLine = statRaw.match(/^cpu\s+(.+)$/m);
			if (cpuLine) {
				var parts = cpuLine[1].trim().split(/\s+/);
				var total = 0;
				for (var j = 0; j < parts.length; j++) total += parseInt(parts[j], 10) || 0;
				var idle = parseInt(parts[3], 10) || 0;
				if (total > 0) {
					cpuUsedPct = ((total - idle) / total * 100).toFixed(1);
					cpuIdlePct = (idle / total * 100).toFixed(1);
				}
			}
		}

		var body = E('div', { 'class': 'cbi-section' }, [
			E('h2', { 'class': 'cbi-section-title' }, _('BBMD Status')),
		]);

		var statusIcon = processRunning ? '\u25CF' : '\u25CB';
		var statusText = processRunning ? _('Running') : _('Stopped');
		var statusStyle = processRunning ? 'color:#188038' : 'color:#d93025';
		var statusSpan = E('span', { 'style': statusStyle + ';font-weight:bold' }, statusIcon + ' ' + statusText);

		body.appendChild(E('div', { 'class': 'cbi-map-description' }, [
			E('p', {}, [
				E('strong', {}, _('Daemon Status: ')),
				statusSpan,
				processRunning ? E('span', { 'style': 'margin-left:12px;color:#666' }, _('PID file: /var/run/bbmd.pid')) : null
			])
		]));

		var section = function(title, rows) {
			var s = E('div', { 'class': 'cbi-section', 'style': 'margin-top:16px' }, [
				E('h3', { 'class': 'cbi-section-title' }, title),
				E('table', { 'class': 'cbi-section-table' }, rows)
			]);
			return s;
		};

		body.appendChild(section(_('Device Identity'), [
			field(_('Device Instance'), uciGet(globalsSection, 'device_instance', '4194303')),
			field(_('Device Name'), uciGet(globalsSection, 'device_name', 'OpenWrt Router')),
			field(_('Model Name'), uciGet(globalsSection, 'model_name', 'OpenWrt BBMD')),
			field(_('Firmware Revision'), uciGet(globalsSection, 'firmware_rev', '1.0.0')),
			field(_('Vendor ID'), uciGet(globalsSection, 'vendor_id', '0')),
			field(_('Network Number'), uciGet(globalsSection, 'network_number', '1')),
		]));

		var modeRows = [
			field(_('Hub Mode'), hubEnabled ? _('Enabled') : _('Disabled')),
			field(_('Node Mode'), nodeEnabled ? _('Enabled') : _('Disabled')),
			field(_('Telemetry'), telemetryEnabled ? _('Enabled') : _('Disabled')),
			field(_('BBMD Service'), bbmdEnabled ? _('Enabled') : _('Disabled')),
			field(_('Log Level'), uciGet(loggingSection, 'level', 'info')),
		];
		if (hubEnabled)
			modeRows.push(field(_('Hub Port'), uciGet(hubSection, 'port', '443')));
		if (nodeEnabled)
			modeRows.push(field(_('Primary Hub URI'), uciGet(nodeSection, 'primary_hub', '-')));

		body.appendChild(section(_('Active Modes'), modeRows));

		var systemRows = [
			field(_('System Uptime'), uptimeStr),
			field(_('CPU Usage'), cpuUsedPct + '% (' + _('Idle') + ': ' + cpuIdlePct + '%)'),
			field(_('Total Memory'), fmtBytes(memTotal)),
			field(_('Available Memory'), fmtBytes(memAvailable)),
			field(_('Memory Usage'), fmtBytes(memUsed) + ' (' + memPct + '%)'),
		];
		body.appendChild(section(_('System Status'), systemRows));

		if (hubEnabled) {
			body.appendChild(section(_('Hub Configuration'), [
				field(_('Hub Port'), uciGet(hubSection, 'port', '443')),
				field(_('Max Nodes'), uciGet(hubSection, 'max_nodes', '50')),
				field(_('Keepalive Interval (s)'), uciGet(hubSection, 'keepalive_interval', '30')),
				field(_('TLS Certificate'), uciGet(hubSection, 'tls_cert', _('Not configured'))),
				field(_('TLS Key'), uciGet(hubSection, 'tls_key') ? _('Configured') : _('Not configured')),
				field(_('CA Certificate'), uciGet(hubSection, 'ca_cert') ? _('Configured') : _('Not configured')),
			]));
		}

		var statsRows = [
			field(_('Connected Nodes'),
				E('em', { 'style': 'color:#666' }, _('Requires daemon stats API (future)'))),
			field(_('Sent Messages'),
				E('em', { 'style': 'color:#666' }, _('Requires daemon stats API (future)'))),
			field(_('Received Messages'),
				E('em', { 'style': 'color:#666' }, _('Requires daemon stats API (future)'))),
			field(_('Current Hub Port'),
				hubEnabled ? uciGet(hubSection, 'port', '443') : E('em', { 'style': 'color:#666' }, _('Hub mode disabled'))),
			field(_('Max Configured Nodes'),
				hubEnabled ? uciGet(hubSection, 'max_nodes', '50') : E('em', { 'style': 'color:#666' }, _('Hub mode disabled'))),
		];
		body.appendChild(section(_('Connected Nodes & Message Stats'), statsRows));

		return body;
	},

	handleSaveApply: null,
	handleSave: null
});
