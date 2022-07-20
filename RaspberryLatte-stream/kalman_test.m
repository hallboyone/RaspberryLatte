%M = brewlog2mat('brewlog2');
M = brewlog2mat('brewlog.txt');
t = M(:,1)'-M(1,1)';
m = M(:,4)';
m(1) = 0;


Ts = 0.05;

F = [1 Ts; 0 1];
Q = 0.05*[Ts^2 Ts; Ts 1];
%Q = [0.05 1; 1 0.5];
H = [1, 0];
R = 0.05;

a = 0.075;
v = nan(1, 50);
x_filt = zeros(2, numel(m)+1);
f_filt = zeros(1, numel(m)+1);
P = 0.05*eye(2);
for i=1:numel(m)
    x_predict = F*x_filt(:, i);
    P_predict = F*P*(F')+Q;
    
    measurement = m(i);
    kalman_gain = P_predict*(H')/(H*P_predict*(H')+R);
    x_filt(:,i+1) = x_predict + kalman_gain*(measurement-H*x_predict);
    
    f_filt(:,i+1) = (1-a)*f_filt(:,i)+a*x_filt(2,i+1);
    P = (eye(2)-kalman_gain*H)*P*(eye(2)-kalman_gain*H)'+kalman_gain*R*(kalman_gain');
end
figure
subplot(2,1,1)
plot([t', t'], [m', x_filt(1,1:end-1)'])
legend(["Unfiltered Scale Readings", "Filtered Scale Readings"]);
subplot(2,1,2)
plot([t',t'],[x_filt(2,1:end-1)',f_filt(1:end-1)'])

legend(["Filtered Flowrate", "Smoothed Flowrate"]);

figure
plot(t',x_filt(2,1:end-1)')
hold on
for a = 0.005:0.005:0.1
    f_filt = zeros(1, numel(m)+1);
    for i=1:numel(m)
        f_filt(:,i+1) = (1-a)*f_filt(:,i)+a*x_filt(2,i+1);
    end
    plot(t',f_filt(1:end-1)')
end