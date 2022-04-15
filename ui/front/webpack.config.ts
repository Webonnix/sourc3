/* eslint-disable global-require */
/* eslint-disable max-len */
import path from 'path';
import fs from 'fs';
import HtmlWebpackPlugin from 'html-webpack-plugin';
import TsconfigPathsPlugin from 'tsconfig-paths-webpack-plugin';
import MiniCssExtractPlugin from 'mini-css-extract-plugin';
// import CompressionPlugin from 'compression-webpack-plugin';
// import { BundleAnalyzerPlugin } from 'webpack-bundle-analyzer';
// import MomentTimezoneDataPlugin from 'moment-timezone-data-webpack-plugin';
import { CleanWebpackPlugin } from 'clean-webpack-plugin';
import * as webpack from 'webpack';
import { Configuration } from 'webpack';

const HTMLInlineCSSWebpackPlugin = require('html-inline-css-webpack-plugin').default;

type ModeType = 'production' | 'development';

const CopyWebpackPlugin = require('copy-webpack-plugin');

interface IConfig extends Configuration {
  devServer: { [key:string]: any }
}

const lessToJs = require('less-vars-to-js');

const themeVariables = lessToJs(
  fs.readFileSync(path.join(__dirname, './ant-theme-vars.less'), 'utf8')
);

const env:ModeType = <ModeType>process.env.NODE_ENV || 'production';

const mode = process.env.WEB ? 'web' : 'desktop';

console.log('env', env);

const build:IConfig = {
  entry: `./src/apps/${mode}/index.tsx`,
  // bail: true,
  output: {
    path: path.join(__dirname, 'dist'),
    filename: '[name].bundle.js',
    chunkFilename: '[name].js',
    assetModuleFilename: 'assets/[name][ext]',
    // publicPath: '/',
    publicPath: env === 'production' && mode === 'desktop' ? './' : '/'
  },
  mode: process.env.NODE_ENV === 'production' ? 'production' : 'development',
  devtool: 'eval-source-map',
  optimization: {
    nodeEnv: 'production'
  },
  resolve: {
    extensions: ['.ts', '.js', '.tsx', '.scss'],
    plugins: [new TsconfigPathsPlugin()],
    modules: [
      path.join(__dirname, './src/types'),
      path.join(__dirname, 'node_modules')
    ]
  },
  performance: {
    hints: false,
    maxEntrypointSize: 512000,
    maxAssetSize: 512000
  },
  devServer: {
    headers: {
      'Cross-Origin-Embedder-Policy': 'require-corp',
      'Cross-Origin-Opener-Policy': 'same-origin'
    },
    historyApiFallback: true,
    watchFiles: path.join(__dirname, 'src'),
    port: 5000,
    open: true,
    hot: true
  },
  module: {
    rules: [
      {
        test: /\.(js|jsx|tsx|ts)$/,
        exclude: /node_modules/,
        loader: 'babel-loader'
      },
      {
        test: /\.css$/,
        use: [
          'style-loader',
          {
            loader: 'css-loader',
            options: {
              modules: {
                localIdentName: '[name]--[hash:base64:5]'

              },
              sourceMap: true
            }
          }
        ]
      },
      {
        test: /\.(scss)$/,
        use: [
          {
            // Adds CSS to the DOM by injecting a `<style>` tag
            loader: 'style-loader'
          },
          {
            // Interprets `@import` and `url()` like `import/require()` and will resolve them
            loader: 'css-loader',
            options: {
              modules: {
                localIdentName: '[name]--[hash:base64:5]'
              }
            }
          },
          {
            // Loader for webpack to process CSS with PostCSS
            loader: 'postcss-loader'
          },
          {
            // Loads a SASS/SCSS file and compiles it to CSS
            loader: 'sass-loader'
          }
        ]
      },
      {
        test: /\.less$/,
        use: [
          { loader: 'style-loader' },
          { loader: 'css-loader' },
          {
            loader: 'less-loader',
            options: {
              lessOptions: {
                javascriptEnabled: true,
                modifyVars: themeVariables
              }
            }
          }
        ]
      },
      {
        test: /\.(jpg|jpeg|png|gif|mp3|svg|wasm)$/,
        type: 'asset/resource'
        // use: ['file-loader']
      }
    ]
  },
  plugins: [
    // new BundleAnalyzerPlugin(),
    new CopyWebpackPlugin({
      patterns: [
        {
          from: path.join(
            __dirname, './node_modules/beam-wasm-client-masternet/'
          ),
          globOptions: {
            ignore: ['package.json']
          }
        }
      ]
    }),
    new CleanWebpackPlugin(),
    new webpack.ProvidePlugin({
      React: 'react'
    }),
    // new webpack.DefinePlugin({
    //   'process.env.NODE_ENV': JSON.stringify('production')
    // }),
    // new webpack.ContextReplacementPlugin(/moment[/\\]locale$/),
    // new MomentTimezoneDataPlugin({
    //   startYear: 1950,
    //   endYear: 2100,
    //   matchZones: /^America\//
    // }),
    new HtmlWebpackPlugin({
      template: path.join(__dirname, 'src', 'public', 'index.html')
    }),
    // new MiniCssExtractPlugin({
    //   filename: 'styles/[name].css',
    //   chunkFilename: 'styles/[id].css'
    // }),
    new MiniCssExtractPlugin({
      filename: env !== 'production' ? '[name].css' : '[name].[hash].css',
      chunkFilename: env !== 'production' ? '[id].css' : '[id].[hash].css'
    }),
    new HTMLInlineCSSWebpackPlugin()
    // new CompressionPlugin({
    //   include: /\/includes/,
    //   deleteOriginalAssets: true
    // })
  ]
};
export default build;
