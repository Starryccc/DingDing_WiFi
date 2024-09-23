const popup = document.getElementById('popup');
const popupText = document.getElementById('message');
const form = document.getElementById('info_form');
form.onsubmit = async (event) => {
    event.preventDefault();
    const data = new URLSearchParams(new FormData(form));
    const response = await fetch('/', {
        method: 'POST',
        body: data,
    });
    if (response.status === 200) {
        const text = await response.text();
        toast(text);
    } else {
        toast("发送失败 " + response.statusText);
    }
};

function toast(message) {
    popup.style.visibility = 'visible';
    popupText.textContent = message;
    setTimeout(() => {
        popup.style.visibility = 'hidden';
    }, 10000);
}
