// Diretórios do banco de dados
var data_motor_path = 'monitor/motor';
var data_temperature_path = 'monitor/temperature';
var data_humidity_path = 'monitor/humidity';

// Referências aos dados do banco
const database_motor = database.ref(data_motor_path);
const database_temperature = database.ref(data_temperature_path);
const database_humidity = database.ref(data_humidity_path);

// Variáveis para salvar os valores atuais do banco de dados
var motor_position;
var temperature_reading;
var humidity_reading;

// Callbacks assíncronas para ler os dados
database_temperature.on('value', (snapshot) => {
    temperature_reading = snapshot.val();
    console.log(temperature_reading);
    document.getElementById("reading-temperature").innerHTML = temperature_reading;
}, (errorObject) => {
    console.log('The read failed: ' + errorObject.name);
});

database_humidity.on('value', (snapshot) => {
    humidity_reading = snapshot.val();
    console.log(humidity_reading);
    document.getElementById("reading-humidity").innerHTML = humidity_reading;
}, (errorObject) => {
    console.log('The read failed: ' + errorObject.name);
});

database_motor.on('value', (snapshot) => {
    motor_position = snapshot.val();
    console.log(motor_position);
    document.getElementById("servo-pos").innerHTML = motor_position;
    document.getElementById("servo-slider").value = motor_position;
}, (errorObject) => {
    console.log('The read failed: ' + errorObject.name);
});

// Atualiza o valor de posição do motor quando um valor é escolhido no slider
function updateServoPosition(value) {
    var sliderValue = parseInt(value);
    document.getElementById("servo-pos").innerHTML = sliderValue;
    // Atualiza a valor de posição do motor no banco de dados, enviando o valor escolhido no slider
    database_motor.set(sliderValue, (error) => {
        if (error) {
            console.log('Falha ao atualizar valor de posição do motor: ' + error);
        } else {
            console.log('Posição do motor atualizada com sucesso: ' + sliderValue);
        }
    });
}