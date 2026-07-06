// gl-ucode object: modem (stub). The GL-SFT1200 has no cellular hardware; return an empty
// modem set so the app cleanly hides the modem tab instead of erroring.
'use strict';

export default {
	get_info:       function(args, ctx) { return { modem_num: 0, sim_num: 0, modems: [] }; },
	get_status:     function(args, ctx) { return { modem_num: 0, modems: [], status: 0 }; },
	get_cell_tower: function(args, ctx) { return {}; },
	get_sim_info:   function(args, ctx) { return {}; },
	get_signals:    function(args, ctx) { return {}; }
};
