'use strict';

//var vConsole = new VConsole();

const base_url = "http://XXX.XXX.XXX.XXX";

var vue_options = {
    el: "#top",
    data: {
        progress_title: '',

        send_data: '',
        send_data_list: [],
        base_url: base_url,
    },
    computed: {
    },
    methods: {
        ir_start: function(){
            return do_get(this.base_url + "/start", { params: "5000" } )
            .then(json =>{
                console.log(json);
            });
        },
        ir_get: function(){
            return do_get(this.base_url + "/get", {} )
            .then(json =>{
                console.log(json);
                if( json.return_value != "NG" ){
                    var ret = prompt('–¼‘O‚ðŒˆ‚ß‚Ä‚­‚¾‚³‚¢B');
                    if( ret ){
                        this.send_data_list.push({ name: ret, data: json.return_value });
                        this.send_data = json.return_value;
                        localStorage.setItem('send_data_list', JSON.stringify(this.send_data_list));
                    }
                }
            });
        },
        ir_send: function(){
            return do_get(this.base_url + "/send", { params: this.send_data} )
            .then(json =>{
                console.log(json);
            });
        },
        data_select: function(index){
            this.send_data = this.send_data_list[index].data;
        },
        data_delete: function(index){
            this.send_data_list.splice(index, 1);
            this.send_data_list = JSON.parse(JSON.stringify(this.send_data_list));
            localStorage.setItem('send_data_list', JSON.stringify(this.send_data_list));
        },
    },
    created: function(){
    },
    mounted: function(){
        proc_load();

        var list = localStorage.getItem('send_data_list');
        if( list )
            this.send_data_list = JSON.parse(list);
    }
};
vue_add_methods(vue_options, methods_utils);
var vue = new Vue( vue_options );

function do_get(url, qs){
    var headers = { 'Content-Type': 'application/x-www-form-urlencoded' }; 

    var params = new URLSearchParams();
    for( var key in qs )
        params.set(key, qs[key] );

    return fetch(url + '?' + params.toString(), {
        method : 'GET',
        headers: headers
    })
    .then((response) => {
        if( !response.ok )
            throw "status is not 200.";
        return response.json();
    });
}