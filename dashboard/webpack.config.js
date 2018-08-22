const path = require('path');
const webpack = require('webpack');
const HtmlWebpackPlugin = require('html-webpack-plugin');

const basePlugins = [
  new webpack.DefinePlugin({
    __DEV__: process.env.NODE_ENV !== 'production',
    __PRODUCTION__: process.env.NODE_ENV === 'production',
    'process.env.NODE_ENV': JSON.stringify(process.env.NODE_ENV),
  }),
];

const clientHTMLPlugin = [
  new HtmlWebpackPlugin({
    template: './public/index.html',
    inject: 'body',
  }),
];

const devPlugins = [
  new webpack.NoEmitOnErrorsPlugin(),
];

const prodPlugins = [
  new webpack.optimize.OccurrenceOrderPlugin(),
];

let plugins =
  basePlugins.concat(process.env.NODE_ENV === 'production' ? prodPlugins : [])
    .concat(process.env.NODE_ENV === 'development' ? devPlugins : []);

const serverConfig = {
  entry: {
    app: './src/server.js',
    vendor: [
      'react',
    ],
  },
  target: 'node',
  output: {
    path: path.join(__dirname, 'dist'),
    filename: '[name].js',
    publicPath: '/',
    sourceMapFilename: '[name].js.map',
    chunkFilename: '[id].chunk.js',
  },
  resolve: {
    extensions: ['.js', '.jsx'],
  },
  devtool: 'source-map',
  plugins,
  module: {
    rules: [
      {test: /\.(js|jsx)$/, enforce: 'pre', loader: 'source-map-loader'},
      {test: /\.(js|jsx)$/, enforce: 'pre',loader: 'eslint-loader'},
      {test: /\.css$/, loader: 'style-loader!css-loader' },
      {test: /\.(js|jsx)$/, loaders: ['babel-loader'], exclude: /node_modules/},
      {test: /\.(png|jpg|jpeg|gif)$/, loader: 'url-loader?prefix=img/&limit=5000'},
      {test: /\.eot(\?v=\d+\.\d+\.\d+)?$/, loader: 'file-loader'},
      {test: /\.(woff)$/, loader: 'url-loader?prefix=font/&limit=5000'},
      {test: /\.ttf(\?v=\d+\.\d+\.\d+)?$/, loader: 'url-loader?limit=10000&mimetype=application/octet-stream'},
      {test: /\.svg(\?v=\d+\.\d+\.\d+)?$/, loader: 'raw-loader'},
      {test: /\.woff(\?v=\d+\.\d+\.\d+)?$/, loader: 'url-loader?limit=10000&minetype=application/font-wof'},
      {test: /\.woff2(\?v=\d+\.\d+\.\d+)?$/, loader: 'url-loader?limit=10000&minetype=application/font-woff2'},
    ],
  },
};

plugins = plugins.concat(clientHTMLPlugin);

const clientConfig = {
  entry: {
    app: './src/index.js',
    vendor: [
      'react',
    ],
  },
  target: 'web', // <=== can be omitted as default is 'web'
  output: {
    path: path.join(__dirname, 'dist'),
    filename: '[name].[hash].js',
    publicPath: '/',
    sourceMapFilename: '[name].[hash].js.map',
    chunkFilename: '[id].chunk.js',
  },
  resolve: {
    extensions: ['*', '.js', '.jsx'],
  },
  devtool: 'source-map',
  plugins,
  module: {
    rules: [
      {test: /\.(js|jsx)$/, enforce: 'pre', loader: 'source-map-loader'},
      {test: /\.(js|jsx)$/, enforce: 'pre',loader: 'eslint-loader'},
      {test: /\.css$/, loader: 'style-loader!css-loader' },
      {test: /\.(js|jsx)$/, loaders: ['babel-loader'], exclude: /node_modules/},
      {test: /\.(png|jpg|jpeg|gif)$/, loader: 'url-loader?prefix=img/&limit=5000'},
      {test: /\.eot(\?v=\d+\.\d+\.\d+)?$/, loader: 'file-loader'},
      {test: /\.(woff)$/, loader: 'url-loader?prefix=font/&limit=5000'},
      {test: /\.ttf(\?v=\d+\.\d+\.\d+)?$/, loader: 'url-loader?limit=10000&mimetype=application/octet-stream'},
      {test: /\.svg(\?v=\d+\.\d+\.\d+)?$/, loader: 'raw-loader'},
      {test: /\.woff(\?v=\d+\.\d+\.\d+)?$/, loader: 'url-loader?limit=10000&minetype=application/font-wof'},
      {test: /\.woff2(\?v=\d+\.\d+\.\d+)?$/, loader: 'url-loader?limit=10000&minetype=application/font-woff2'},
    ],
  },
};

module.exports = [serverConfig, clientConfig];
